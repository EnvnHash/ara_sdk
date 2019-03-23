//
//  SNTPartLinesFog.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTPartLinesFog.h"

namespace tav
{
    SNTPartLinesFog::SNTPartLinesFog(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
    	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

    	getChanCols(&chanCols);

    	pa = (PAudio*) scd->pa;
    	kin = static_cast<KinectInput*>(scd->kin);

        ps = new GLSLParticleSystem(shCol, 8000, scd->screenWidth, scd->screenHeight);
        ps->setFriction(0.3f);
        ps->setLifeTime(0.4f);
        ps->setAgeFading(true);
        ps->setColorPal(chanCols);

      //  nis = kin->getNis();
        emitPos = glm::vec3(0.f, 0.f, 0.f);
        emitDir = glm::vec3(0.f, 0.f, 0.f);
        
        partPerSamp = 40;
        pllRange = 0.25f;
        pllPtrOffs = 0.2f;
        offX = 0.05f;
        offY = 0.15f;
        dirOff = 0.001f;
        posRand = 0.01f;
        
        headOffs = 0.0f;
        headHeightScale = 1.8f;
                
        pllPtr = new float[scd->nrChannels];
        for (auto i=0;i<scd->nrChannels;i++)
            pllPtr[i] = 0.f;
    }
    
    
    SNTPartLinesFog::~SNTPartLinesFog()
    {
        delete ps;
        delete pllPtr;
    }
    
    
    void SNTPartLinesFog::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->disableDepthTest();
            _tfo->setSceneNodeColors(chanCols);
        }
        
        if(!isInited)
        {
            ps->init(_tfo);
        	isInited = true;
        }

        useTextureUnitInd(0, tex0->getId(), _shader, _tfo); // zum tfo aufnehmen der texturen
        ps->drawToBlend(GLSLParticleSystem::QUADS, _tfo);
        
    }
    
    
    void SNTPartLinesFog::update(double time, double dt)
    {
        /*
        // get new position
        if (kin->isNisInited())
        {
            if (frameNr != kin->getNisFrameNr())
            {
                frameNr = kin->getNisFrameNr();

               // kin->lockDepthMtx();
                
                short usr = nis->getNearestUser();
                if(usr >= 0)
                {
                    rightH = nis->getJoint(nis->getNearestUser(), nite::JOINT_RIGHT_HAND);
                    leftH = nis->getJoint(nis->getNearestUser(), nite::JOINT_LEFT_HAND);
                    center = nis->getJoint(nis->getNearestUser(), nite::JOINT_TORSO);
                    head = nis->getJoint(nis->getNearestUser(), nite::JOINT_HEAD);
                    foot = nis->getJoint(nis->getNearestUser(), nite::JOINT_LEFT_FOOT);
                    
                    length = fmax((head.y - headOffs) * headHeightScale, 0.0f);
                    
                    emitPos = center;
                    
                    emitDir = rightH - leftH;
                    emitDir.z *= 8.f;
                    lengthHands = std::fabs( std::sqrt( (emitDir.x*emitDir.x) + (emitDir.z*emitDir.z) ) );
                    lengthHands = std::fmin(lengthHands, 1.f) * 0.02f;
                    emitDir = normalize(emitDir);
                    
                    angle = std::atan2(emitDir.y, emitDir.z);
                    if (!std::isfinite(angle)) angle = 0.f;
                    
                    // act = nis->getAvgAct(usr) * 1.5f;
                    
                    res = glm::vec3(emitPos.x - 0.15f, (1.f - emitPos.z) * 4.5f - 2.5f, 0.f);
                }
                
                //kin->unLockDepthMtx();
            }
        }
        */
        
        //if ( pa->isPlaying )
        if ( pa->isPlaying && lastBlock != pa->waveData.blockCounter )
        {
            for (auto chanNr=0;chanNr<scd->nrChannels;chanNr++)
            {
                float col = static_cast<float>(chanNr) / static_cast<float>(std::max(scd->nrChannels-1, 1));

                for (auto i=0;i<partPerSamp;i++)
                {
                    float fInd = static_cast<float>(i) / static_cast<float>(partPerSamp) * pllRange;
                    audioPos = glm::vec3(pa->getPllAtPos(scd->chanMap[chanNr], pllPtr[chanNr] +offX +fInd),
                                    pa->getPllAtPos(scd->chanMap[chanNr], pllPtr[chanNr] +offY +fInd),
                                    0.f);
                    
                    audioDir = glm::vec3(pa->getPllAtPos(scd->chanMap[chanNr], pllPtr[chanNr] +offX +fInd +dirOff),
                                    pa->getPllAtPos(scd->chanMap[chanNr], pllPtr[chanNr] +offY +fInd +dirOff),
                                    0.f) - emitPos;

//                    data.emitOrg = res;
                    data.emitOrg = audioPos * 2.f * length + res;
                    data.emitVel = glm::normalize(audioDir);
                    data.size = lengthHands;
                    data.angle = angle + (0.2f + getRandF(-1.f, 1.f)) * TWO_PI;;
                    data.posRand = posRand;
                    //data.dirRand = 0.1f;
                    data.colInd = col;
                    data.texNr = static_cast<int>(getRandF(0, 8.f)); // bezieht sich auf den indice des tex arrays
                    
                    ps->emit(1, data);
                }
                
                pllPtr[chanNr] = (float) fmod(pllPtr[chanNr] + pllPtrOffs * dt, 1.0);
            }
            
            lastBlock = pa->waveData.blockCounter;
        }
        
        ps->update(time);
    }
    
}
