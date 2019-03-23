//
// SNTDrops.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTDrops.h"

namespace tav
{
    SNTDrops::SNTDrops(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
    	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
    	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];

    	getChanCols(&chanCols);

    	pa = (PAudio*) scd->pa;
    	kin = static_cast<KinectInput*>(scd->kin);

        ps = new GLSLParticleSystem(shCol, 100, _scd->screenWidth, _scd->screenHeight);
        ps->setFriction(0.6f);
        ps->setLifeTime(1.5f);
        ps->setAgeFading(true);
        ps->setColorPal(chanCols);

       // nis = kin->getNis();
        emitPos = glm::vec3(0.f, 0.f, 0.f);
        emitDir = glm::vec3(0.f, 0.f, 0.f);
        
        sizeMod = 0.04f;
        sizeRand = 0.5f;
        
        distHandOffset = 1.f;
        distHandScale = 8.f;
        
        posSel = new glm::vec3[3];
    }
    
    SNTDrops::~SNTDrops()
    {
        delete ps;
    }

    void SNTDrops::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo)
            _tfo->setSceneNodeColors(chanCols);

        if(!isInited)
        {
            ps->init(_tfo);
        	isInited = true;
        }

        for (auto i=0;i<4;i++)
            useTextureUnitInd(i, scd->tex[i], _shader, _tfo); // zum tfo aufnehmen der texturen
        ps->drawToBlend(GLSLParticleSystem::QUADS, _tfo);

        if (_tfo) _tfo->disableDepthTest();
    }
    
    void SNTDrops::update(double time, double dt)
    {
        float act = 0.f;
  
        if ( lastBlock != pa->waveData.blockCounter || repeat)
        {
            // muss eigentlich mindestens zweimal gezeichnet werden, damit der backbuffer auch das bild
            // kriegt und das feedback nicht flimmert
            if (pa->getCatchedOnset(scd->chanMap[0]) > 0.0f || repeat)
            {
                data.emitOrg = res;
                data.emitVel = emitDir * 3.f;
                data.size = (1.f + getRandF(-0.5f, 3.f)) * pa->getCatchedAmp(scd->chanMap[0]) * sizeMod;
                data.angle = (0.2f + getRandF(-1.f, 1.f)) * TWO_PI;
                data.texNr = static_cast<int>(getRandF(0, 8.f)); // bezieht sich auf den indice des tex arrays
                data.life = 2.f;
                data.life *= getRandF(0.8f, 1.2f);
//                data.colInd = std::fmin(std::fmax(distHandHip + getRandF(0.0f, 0.4f), 0.f), 1.f);
                data.colInd = getRandF(0.f, 1.f);

                if (!repeat) {
                    ps->emit(1, data);
                    ps->copyEmitData(data, oldData);
                } else {
                    ps->emit(1, oldData);
                }
                repeat = !repeat;
            }
        }

        ps->update(time);
    }
}
