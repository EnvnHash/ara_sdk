//
// SNTPartLines.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTPartLines.h"

using namespace glm;
namespace tav
{
    SNTPartLines::SNTPartLines(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	getChanCols(&chanCols);

    	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
    	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

    	pa = (PAudio*) scd->pa;
    	kin = static_cast<KinectInput*>(scd->kin);

        ps = new GLSLParticleSystem(shCol, 4000, scd->screenWidth, scd->screenHeight);
        ps->setFriction(0.3f);
        ps->setLifeTime(0.4f);
        ps->setAgeFading(true);
        ps->setColorPal(chanCols);

        //nis = kin->getNis();
        emitPos = glm::vec3(0.f, 0.f, 0.f);
        emitDir = glm::vec3(0.f, 0.f, 0.f);
        
        partPerSamp = 40;
        pllRange = 0.25f;
        pllPtrOffs = 0.2f;
        offX = 0.05f;
        offY = 0.15f;
        dirOff = 0.001f;
        
        headOffs = 0.0f;
        headHeightScale = 1.8f;
        
        pllPtr = new float[scd->nrChannels];
        for (auto i=0;i<scd->nrChannels;i++)
            pllPtr[i] = 0.f;
    }
    
    SNTPartLines::~SNTPartLines()
    {
        delete ps;
        delete pllPtr;
    }
    
    void SNTPartLines::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->disableDepthTest();
            _tfo->setSceneNodeColors(chanCols);
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);
        }
        
        if(!isInited)
        {
            ps->init(_tfo);
        	isInited = true;
        }

        useTextureUnitInd(0, tex0->getId(), _shader, _tfo); // zum tfo aufnehmen der texturen
        ps->drawToBlend(GLSLParticleSystem::QUADS, _tfo);
    }
    
    void SNTPartLines::update(double time, double dt)
    {

        if ( pa->isPlaying && lastBlock != pa->waveData.blockCounter )
        {
            for (auto chanNr=0;chanNr<scd->nrChannels;chanNr++)
            {
                float col = static_cast<float>(chanNr) / static_cast<float>(std::max(scd->nrChannels-1, 1));

                //cout << scd->chanMap[chanNr] << std::endl;
                
                for (auto i=0;i<partPerSamp;i++)
                {
                    float fInd = static_cast<float>(i) / static_cast<float>(partPerSamp) * pllRange;
                    audioPos = glm::vec3(pa->getPllAtPos(scd->chanMap[chanNr], pllPtr[chanNr] +offX +fInd),
                                    pa->getPllAtPos(scd->chanMap[chanNr], pllPtr[chanNr] +offY +fInd),
                                    0.f);
                    
                    audioDir = glm::vec3(pa->getPllAtPos(scd->chanMap[chanNr], pllPtr[chanNr] +offX +fInd +dirOff),
                                    pa->getPllAtPos(scd->chanMap[chanNr], pllPtr[chanNr] +offY +fInd +dirOff),
                                    0.f) - emitPos;

                    data.emitOrg = audioPos * 2.6f * length + res;
                    data.emitVel = normalize(audioDir);
                    data.speed = 0.0f;
                    data.size = lengthHands * 2.f;
                    data.angle = angle + (0.2f + getRandF(-1.f, 1.f)) * TWO_PI;
                    //data.dirRand = 0.2f;
                    data.texUnit = 1;
                    data.texNr = static_cast<int>(getRandF(0, 8.f)); // bezieht sich auf den indice des tex arrays
                    data.colInd = col;
                    
                    ps->emit(1, data);
                }
                
                pllPtr[chanNr] = (float) fmod(pllPtr[chanNr] + pllPtrOffs * dt, 1.0);
            }
            
            lastBlock = pa->waveData.blockCounter;
        }
        
        ps->update(time);
    }
    

}
