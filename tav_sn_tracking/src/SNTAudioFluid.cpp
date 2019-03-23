//
// SNTAudioFluid.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTAudioFluid.h"

namespace tav
{
    SNTAudioFluid::SNTAudioFluid(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
    	pa = (PAudio*) scd->pa;
    	kin = static_cast<KinectInput*>(scd->kin);
    	kinMapping = static_cast<kinectMapping*>(scd->kinMapping);

    	getChanCols(&chanCols);

        flWidth = 300;
        flHeight = 300;
        
        posScale = 1.25f;
        ampScale = 12.f;
        
        forceScale = glm::vec2(static_cast<float>(flWidth) * 0.5f, static_cast<float>(flHeight) * 0.85f);
        alphaScale = 0.01f;
        
        fluidSim = new GLSLFluid(false, shCol);
        fluidSim->allocate(flWidth, flHeight, 0.5f);
        fluidSim->dissipation = 0.99f;
        fluidSim->velocityDissipation = 0.99f;
        fluidSim->setGravity(glm::vec2(0.0f,0.0f));
        
        oldPos = new glm::vec2[scd->nrChannels];
        
        quadAr = new QuadArray(40, 40, -1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f));

        shdr = shCol->getStdTex();

        nis = kin->getNis();
    }
    
    
    SNTAudioFluid::~SNTAudioFluid()
    {
        fluidSim->cleanUp();
        delete fluidSim;
        delete quadAr;
        delete shdr;
    }
    

    void SNTAudioFluid::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo)
        {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        sendStdShaderInit(_shader);
//        useTextureUnitInd(0, fluidSim->obstaclesFbo->getColorImg(), _shader, _tfo);
        useTextureUnitInd(0, fluidSim->getResTex(), _shader, _tfo);

        quadAr->draw(_tfo);
//        fluidSim->draw();

        /*
        glActiveTexture(GL_TEXTURE0);
        shdr->begin();
        
        shdr->setUniform1i("tex", 0);
        shdr->setIdentMatrix4fv("m_pvm");
        
        glBindTexture(GL_TEXTURE_2D, kin->getShdwTexId());
        quadAr->draw(_tfo);
        
        shdr->end();
         */
    }
    
    
    void SNTAudioFluid::update(double time, double dt)
    {
        glm::vec2 pos, vel, c;
        glm::vec4 col;
        float amp, readPos, readPos2;
        
        if ( lastBlock != pa->waveData.blockCounter )
        {
            readPos = (std::cos(time * 0.05f) + 1.f) * 0.5f;
            
            // Adding temporal Force, in pixel relative to flWidth and flHeight
            for (auto cNr=0;cNr<scd->nrChannels;cNr++)
            {
                readPos2 = std::fmod(readPos + (std::cos(time * 0.02f + (float)cNr / (float)scd->nrChannels) + 1.f) * 0.1f, 1.f);
                
                pos = glm::vec2(std::fmax(std::fmin((pa->getPllAtPos(scd->chanMap[cNr], readPos) * posScale + 1.f) * 0.5f, 0.99f), 0.01f) * flWidth,
                           std::fmax(std::fmin((pa->getPllAtPos(scd->chanMap[cNr], readPos2) * posScale + 1.f) * 0.5f, 0.99f), 0.01f) * flHeight);
                vel = (pos - oldPos[cNr]) * 4.f;
                oldPos[cNr] = pos;
                c = glm::normalize(forceScale - pos);
                
                col = chanCols[cNr % MAX_NUM_COL_SCENE];
                amp = std::fmin(pa->getMedAmp(scd->chanMap[cNr]) * ampScale, 1.f);
                eDen = amp * 0.2f;
                eTemp = amp * 10.f;
                eRad = amp * 5.f + 0.9f;
                //col.a = amp * alphaScale;
                fluidSim->addTemporalForce(pos, vel, col, eRad, eTemp, eDen);
            }
            
            fluidSim->update();            
            lastBlock = pa->waveData.blockCounter;
        }

        // add users
        if (kin->isNisInited())
        {
            if (frameNr != kin->getShadowFrameNr())
            {
                frameNr = kin->getShadowFrameNr();

                short usr = nis->getNearestUser();
                
                if (usr >= 0)
                {                    
                    rightH = glm::vec2(((nis->getJoint(nis->getNearestUser(), nite::JOINT_RIGHT_HAND).x + 1.f) *0.5f
                                  *kinMapping->scale->x + kinMapping->offset->x) *flWidth,
                                  ((1.f - nis->getJoint(nis->getNearestUser(), nite::JOINT_RIGHT_HAND).z)
                                  *kinMapping->scale->y + kinMapping->offset->y) *flHeight);
                    
                    leftH = glm::vec2(((nis->getJoint(nis->getNearestUser(), nite::JOINT_LEFT_HAND).x + 1.f) *0.5f
                                 *kinMapping->scale->x + kinMapping->offset->x) *flWidth,
                                 ((1.f - nis->getJoint(nis->getNearestUser(), nite::JOINT_LEFT_HAND).z)
                                 *kinMapping->scale->y + kinMapping->offset->y) *flHeight);
                    
                    velL = (leftH - oldLeftH) * 0.5f;
                    velR = (rightH - oldRightH) * 0.5f;
                    
                    eDen = 0.2f;
                    eTemp = 10.f;
                    eRad = 5.9f;
                    col = glm::vec4(1.f, 1.f, 1.f, 1.f);
                    
                    fluidSim->addTemporalForce(rightH, velR, col, eRad, eTemp, eDen);
                    fluidSim->addTemporalForce(leftH, velL, col, eRad, eTemp, eDen);
                    
                    oldRightH = rightH;
                    oldLeftH = leftH;
                }
                
                kin->uploadShadowImg();
            }
        }
    }
}
