//
// SNTAudioFluidDepthFlow.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTAudioFluidDepthFlow.h"

namespace tav
{
    SNTAudioFluidDepthFlow::SNTAudioFluidDepthFlow(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	getChanCols(&chanCols);

    	pa = (PAudio*) scd->pa;
    	kin = static_cast<KinectInput*>(scd->kin);
    	kinMapping = static_cast<kinectMapping*>(scd->kinMapping);
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        flWidth = 250;
        flHeight = 250;
        
        posScale = 1.f;
        ampScale = 10.f;
        
        forceScale = glm::vec2(static_cast<float>(flWidth) * 0.5f, static_cast<float>(flHeight) * 0.5f);
        alphaScale = 0.01f;
        
        fluidSim = new GLSLFluid(false, shCol);
        fluidSim->allocate(flWidth, flHeight, 0.5f);
        fluidSim->dissipation = 0.999f;
        fluidSim->velocityDissipation = 0.999f;
        fluidSim->setGravity(glm::vec2(0.0f,0.0f));

        optflow = new GLSLOpticalFlow(shCol, kin->getDepthWidth(), kin->getDepthHeight());
        optflow->setBright(-5.0f);
        blur = new FastBlurMem(0.6f, shCol, kin->getDepthWidth(), kin->getDepthHeight());
        
        oldPos = new glm::vec2[scd->nrChannels];
        
        quadAr = new QuadArray(40, 40, -1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f));
        quadAr->rotate(float(M_PI) * 0.5f, 1.f, 0.f, 0.f);
//        quadAr->translate(0.f, f, 0.f);

        shdr = shCol->getStdTex();
        kin->setUpdateShadow(true);
    }
    

    void SNTAudioFluidDepthFlow::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo)
        {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            _tfo->setSceneNodeColors(chanCols);
        }
        
        sendStdShaderInit(_shader);
        //useTextureUnitInd(0, kin->getShdwTexId(), _shader, _tfo);
        //useTextureUnitInd(0, blur->getResult(), _shader, _tfo);
//        useTextureUnitInd(0, optflow->getResTexId(), _shader, _tfo);
//        useTextureUnitInd(0, fluidSim->getVelocityTex(), _shader, _tfo);
        useTextureUnitInd(0, fluidSim->getResTex(), _shader, _tfo);

        quadAr->draw(_tfo);
    }
    
    
    void SNTAudioFluidDepthFlow::update(double time, double dt)
    {
        glm::vec2 pos, vel, c;
        glm::vec4 col;
        float amp, readPos, readPos2;
        glm::vec3 shdwCol = glm::vec3(1.f, 0.9f, 0.95f);
        
        kin->uploadShadowImg();

        if ( lastBlock != pa->waveData.blockCounter )
        {
            readPos = (std::cos(time * 0.05f) + 1.f) * 0.5f;
            
            // Adding temporal Force, in pixel relative to flWidth and flHeight
            for (auto cNr=0;cNr<scd->nrChannels;cNr++)
            {
                readPos2 = std::fmod(readPos + (std::cos(time * 0.02f + (float)cNr / (float)scd->nrChannels) + 1.f) * 0.1f, 1.f);
                
                pos = glm::vec2(std::fmax(std::fmin( (pa->getPllAtPos(scd->chanMap[cNr], readPos)
                                                     * posScale + 1.f) * 0.5f, 0.99f), 0.01f) * flWidth,
                                std::fmax(std::fmin( (pa->getPllAtPos(scd->chanMap[cNr], readPos2)
                                                     * posScale + 1.f) * 0.5f, 0.99f), 0.01f) * flHeight);
                
                if(kin->getShadowUserCenter(0))
                    pos += glm::vec2( ((*kin->getShadowUserCenter(0)).x - 0.5f) * flWidth,
                                      ((*kin->getShadowUserCenter(0)).y - 0.5f) * flHeight );
                
                vel = (pos - oldPos[cNr]) * 4.f;
                oldPos[cNr] = pos;
                c = glm::normalize(forceScale - pos);
                
                col = chanCols[cNr % MAX_NUM_COL_SCENE];
                amp = std::fmin(pa->getMedAmp(scd->chanMap[cNr]) * ampScale, 1.f);
                eDen = amp * 0.2f;
                eTemp = amp * 10.f;
                eRad = amp * 2.f + 0.9f;
                //col.a = amp * alphaScale;
                
                fluidSim->addTemporalForce(pos, vel, col, eRad, eTemp, eDen);
            }
            
            lastBlock = pa->waveData.blockCounter;
        }

        // add users
        if (kin->isNisInited())
        {
            if (frameNr != kin->getShadowFrameNr())
            {
                // blur the shadow texture
                blur->proc( kin->getShdwTexId() );
                
                // apply optical flow;
                optflow->update( blur->getResult(), blur->getLastResult() );
                
                // apply optflow to fluidsim
                fluidSim->addVelocity(optflow->getResTexId());
                fluidSim->addColor(blur->getResult(), shdwCol, 1.f, true);
                
                frameNr = kin->getShadowFrameNr();
            }
        }
        
        fluidSim->update();
    }
    
    
    SNTAudioFluidDepthFlow::~SNTAudioFluidDepthFlow()
    {
        fluidSim->cleanUp();
        delete fluidSim;
        delete quadAr;
        delete shdr;
    }
}
