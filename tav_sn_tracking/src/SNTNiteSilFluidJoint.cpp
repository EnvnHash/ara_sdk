//
//  SNTNiteSilFluidJoint.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTNiteSilFluidJoint.h"

namespace tav
{
    SNTNiteSilFluidJoint::SNTNiteSilFluidJoint(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	kin = static_cast<KinectInput*>(scd->kin);
    	osc = static_cast<OSCData*>(scd->osc);
    	shdr = shCol->getStdTex();
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        scrScale = 2.f;
        
        rawQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
                           glm::vec3(0.f, 0.f, 1.f),
                           0.f, 0.f, 0.f, 0.f);    // color will be replace when rendering with blending on
        
        rotQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
                           glm::vec3(0.f, 0.f, 1.f),
                           0.f, 0.f, 0.f, 0.f,
                           nullptr, 1, true);    // color will be replace when rendering with blending on
        
        userMapTex = new TextureManager();
        kinColorTex = new TextureManager*[2];
        for(short i=0;i<2;i++)
            kinColorTex[i] = new TextureManager();
        
        edgeDetect = shCol->addCheckShader("edgeDetect", "shaders/edgeDetect.vert",
                                                              "shaders/edgeDetect.frag");
        edgeDetect->link();
        
        flWidth = static_cast<float>(_scd->screenWidth) / scrScale;
        flHeight = static_cast<float>(_scd->screenHeight) / scrScale;
        
        forceScale = glm::vec2(flWidth * 0.5f, flHeight * 0.85f);
        alphaScale = 0.01f;
        
        fluidSim = new GLSLFluid(false, shCol);
        fluidSim->allocate(static_cast<int>(flWidth), static_cast<int>(flHeight), 0.5f);
        fluidSim->dissipation = 0.998f;
        fluidSim->velocityDissipation = 0.99f;
        fluidSim->setGravity(glm::vec2(0.0f,0.0f));
        
        //quadAr = new QuadArray(40, 40, -1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f));
        
        multCol = glm::vec3(1.f, 0.f, 1.f);
        nis = kin->getNis();
        
        nrForceJoints = 15;
        oldJointPos = new glm::vec2[nrForceJoints];
        
        forceJoints = new nite::JointType[nrForceJoints];
        forceJoints[0] = nite::JOINT_HEAD;
        forceJoints[1] = nite::JOINT_NECK;
        
        forceJoints[2] = nite::JOINT_LEFT_SHOULDER;
        forceJoints[3] = nite::JOINT_RIGHT_SHOULDER;
        forceJoints[4] = nite::JOINT_LEFT_ELBOW;
        forceJoints[5] = nite::JOINT_RIGHT_ELBOW;
        forceJoints[6] = nite::JOINT_LEFT_HAND;
        forceJoints[7] = nite::JOINT_RIGHT_HAND;
        
        forceJoints[8] = nite::JOINT_TORSO;
        
        forceJoints[9] = nite::JOINT_LEFT_HIP;
        forceJoints[10] = nite::JOINT_RIGHT_HIP;
        forceJoints[11] = nite::JOINT_LEFT_KNEE;
        forceJoints[12] = nite::JOINT_RIGHT_KNEE;
        forceJoints[13] = nite::JOINT_LEFT_FOOT;
        forceJoints[14] = nite::JOINT_RIGHT_FOOT;
    }
    
    
    
    SNTNiteSilFluidJoint::~SNTNiteSilFluidJoint()
    {
        fluidSim->cleanUp();
        delete fluidSim;
        delete rawQuad;
        delete rotQuad;
        //        delete quadAr;
        delete shdr;
    }
    

    void SNTNiteSilFluidJoint::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo)
        {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);
            _tfo->end();
            glDisable(GL_RASTERIZER_DISCARD);
        }
        
        if (!inited)
        {
            userMapTex->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
                                 GL_R8, GL_RED, GL_TEXTURE_2D, GL_UNSIGNED_BYTE); // for the userMap
            for(short i=0;i<2;i++)
                kinColorTex[i]->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
                                         GL_RGB8, GL_RGB, GL_TEXTURE_2D, GL_UNSIGNED_BYTE); // for the userMap
            userMapConv = new uint8_t[kin->getDepthWidth() * kin->getDepthHeight()];
            memset(userMapConv, 0, kin->getDepthWidth() * kin->getDepthHeight());
            edgePP = new PingPongFbo(shCol, kin->getDepthWidth(),
                                     kin->getDepthHeight(), GL_RGB8, GL_TEXTURE_2D);
            optFlow = new GLSLOpticalFlow(shCol, kin->getDepthWidth(),
                                          kin->getDepthHeight());
            optFlow->setMedian(10.f);
            inited = true;
            
        } else
        {
            //sendStdShaderInit(_shader);
            //useTextureUnitInd(0, fluidSim->obstaclesFbo->getColorImg(), _shader, _tfo);
            //useTextureUnitInd(0, fluidSim->getResTex(), _shader, _tfo);
            
             // Adding temporal Force, in pixel relative to flWidth and flHeight
            if (nis->isInited())
            {
                const nite::Array<nite::UserData>& users = nis->userTrackerFrame.getUsers();
                for (int i=0; i<users.getSize(); ++i)
                {
                    const nite::UserData& user = users[i];
                    
                    if (user.isVisible())
                    {
                        for (int jNr=0;jNr<nrForceJoints;jNr++)
                        {
                            glm::vec2 m = glm::vec2((nis->skelData->getJoint(i, forceJoints[jNr]).x * 0.5f + 0.5f) * flWidth,
                                                   (1.f - (nis->skelData->getJoint(i, forceJoints[jNr]).y * 0.5f + 0.5f)) * flHeight);
                            glm::vec2 d = (m - oldJointPos[jNr]) * (1.f / static_cast<float>(users.getSize() * 2));
                            glm::vec2 c = glm::normalize(forceScale - m);
                            
                            fluidSim->addTemporalVelocity(m, d, 3.0f * (1.f / static_cast<float>(users.getSize())));

                            oldJointPos[jNr] = m;
                        }
                    }
                }
            }

            
            //multCol = glm::vec3(1.f, 0.56f, 0.08f);
                        multCol = glm::vec3((float)std::fmax(cos(time * 0.2 + 0.1) * 0.5f + 0.5f, 0.3f),
                                            (float)std::fmax(cos(time * 0.12 + 0.21) * 0.5f + 0.5f, 0.3f),
                                            (float)std::fmax(cos(time * 0.123 + 0.022) * 0.5f + 0.5f, 0.3f));
            
            fluidSim->addColor(edgePP->getSrcTexId(), multCol, osc->blurFboAlpha * 2.f);
            fluidSim->update();
            
            // render result
            shdr->begin();
            shdr->setUniform1i("tex", 0);
            shdr->setIdentMatrix4fv("m_pvm");
            
            glActiveTexture(GL_TEXTURE0);
            //            glBindTexture(GL_TEXTURE_2D, optFlow->getResTexId());
            //            glBindTexture(GL_TEXTURE_2D, edgeFBO->getColorImg());
            glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());
            rotQuad->draw();
        }
        
        
        if (_tfo)
        {
            glEnable(GL_RASTERIZER_DISCARD);
            _shader->begin();       // extrem wichtig, sonst keine Bindepunkte für TFO!!!
            _tfo->begin(_tfo->getLastMode());
        }
    }
    
    
    
    void SNTNiteSilFluidJoint::update(double time, double dt)
    {
        // add users
        if (kin->isNisInited() && inited)
        {
            // get movement
            if(colFrameNr != kin->getColFrameNr())
            {
                colFrameNr = kin->getColFrameNr();

                kinColorImgPtr = ++kinColorImgPtr % 2;
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, kinColorTex[kinColorImgPtr]->getId());
                glTexSubImage2D(GL_TEXTURE_2D,             // target
                                0,                          // First mipmap level
                                0, 0,                       // x and y offset
                                kin->getDepthWidth(),
                                kin->getDepthHeight(),
                                GL_RGB,
                                GL_UNSIGNED_BYTE,
                                kin->getActColorImg());
                
                optFlow->update(kinColorTex[kinColorImgPtr]->getId(), kinColorTex[(kinColorImgPtr +1) %2]->getId());
            }
            
            // get silhoutte
            if(frameNr != kin->getNisFrameNr())
            {
                frameNr = kin->getNisFrameNr();

                const nite::UserMap& userLabels = nis->getUserMap();
                const nite::UserId* pLabels = userLabels.getPixels();
                
                // convert userLabels
                float factor = 1;
                uint8_t* pTexRow = userMapConv;
                for (int y=0; y < kin->getDepthHeight() ; ++y)
                {
                    uint8_t* pTex = pTexRow; // get the pointer to the first pixel in the actual row
                    for (int x=0; x < kin->getDepthWidth() ; ++x, ++pLabels)
                    {
                        if (*pLabels == 0) factor = 0; else factor = 255.f;
                        *(pTex++) = factor;
                    }
                    pTexRow += kin->getDepthWidth();
                }
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, userMapTex->getId());
                glTexSubImage2D(GL_TEXTURE_2D,             // target
                                0,                          // First mipmap level
                                0, 0,                       // x and y offset
                                kin->getDepthWidth(),
                                kin->getDepthHeight(),
                                GL_RED,
                                GL_UNSIGNED_BYTE,
                                userMapConv);
                
                
                // convert nite silhoutte to edge
                edgePP->dst->bind();
                edgePP->dst->clear();
                
                edgeDetect->begin();
                edgeDetect->setIdentMatrix4fv("m_pvm");
                edgeDetect->setUniform1i("tex", 0);
                edgeDetect->setUniform1f("stepX", 1.f / static_cast<float>(scd->screenWidth));
                edgeDetect->setUniform1f("stepY", 1.f / static_cast<float>(scd->screenHeight));
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, userMapTex->getId());
                
                rawQuad->draw();
                
                edgePP->dst->unbind();
                edgePP->swap();
                
            }
        }
    }
    
    
    
    void SNTNiteSilFluidJoint::onCursor(double xpos, double ypos)
    {
        mouseX = xpos;
        mouseY = ypos;
    }
    

}
