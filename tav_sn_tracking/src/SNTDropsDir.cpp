//
// SNTDropsDir.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTDropsDir.h"

namespace tav
{
    SNTDropsDir::SNTDropsDir(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
    	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

    	getChanCols(&chanCols);

    	kin = static_cast<KinectInput*>(scd->kin);

        ps = new GLSLParticleSystem(shCol, 100,
                                    scd->screenWidth, scd->screenHeight);
        ps->setFriction(0.5f);
        ps->setLifeTime(1.5f);
        ps->setAgeFading(true);
        ps->setColorPal(chanCols);

        //nis = kin->getNis();
        emitPos = glm::vec3(0.f, 0.f, 0.f);
        emitDir = glm::vec3(0.f, 0.f, 0.f);        
    }
    
    
    SNTDropsDir::~SNTDropsDir()
    {
        delete ps;
    }
    
    
    void SNTDropsDir::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
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
    
    
    void SNTDropsDir::update(double time, double dt)
    {
        float act = 0.f;
        /*
        // get new position
        if (kin->isNisInited())
        {
            if (frameNr != kin->getNisFrameNr())
            {
                frameNr = kin->getNisFrameNr();

               // kin->lockDepthMtx();
                
                short usr = nis->getNearestUser();
                if (usr >= 0)
                {
                    rightH = nis->getJoint(nis->getNearestUser(), nite::JOINT_RIGHT_HAND);
                    leftH = nis->getJoint(nis->getNearestUser(), nite::JOINT_LEFT_HAND);
                    rightHip = nis->getJoint(nis->getNearestUser(), nite::JOINT_RIGHT_HIP);
                    center = nis->getJoint(nis->getNearestUser(), nite::JOINT_TORSO);
                    
                    emitDir = rightH - leftH;
                    emitDir.y = emitDir.z * 3.f;
                    emitDir.z = 0.f;
                    
                    emitPos = center;
                    
                    //                emitDir = rightH - rightHip;
                    //                emitDir.z = emitDir.z -1.f;
                    
                    dirSpeed = std::fabs( std::sqrt( (emitDir.x*emitDir.x) + (emitDir.z*emitDir.z) ) );
                    
                    
                    angle = std::atan2(emitDir.y, emitDir.x);
                    angle = 0.f; // richtige richtung?
                    
                    emitDir.x = std::cos(angle);
                    emitDir.y = std::sin(angle);
                    emitDir = normalize(emitDir);
                    
                    act = nis->getAvgAct(usr) * 1.5f;                
                    
                    res = glm::vec3(emitPos.x - 0.15f, (1.f - emitPos.z) * 4.5f - 2.5f, 0.f);
                }
                
               // kin->unLockDepthMtx();
            }
        }
        */
        if ( lastBlock != pa->waveData.blockCounter )
        {
            if (pa->getCatchedOnset(scd->chanMap[0]) > 0.0f)
            {
                data.emitOrg = res;
                data.emitVel = emitDir * 4.f;
                data.speed = dirSpeed;
                data.size = (1.f + getRandF(-0.5f, 3.f)) * pa->getCatchedAmp(scd->chanMap[0]) * 0.05f;
                data.texNr = static_cast<int>(getRandF(0, 8.f)); // bezieht sich auf den indice des tex arrays
                data.angle = (0.2f + getRandF(-1.f, 1.f)) * TWO_PI;
                data.lifeRand = 0.2f;
                data.colInd = getRandF(0.0f, 0.4f);
                
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
