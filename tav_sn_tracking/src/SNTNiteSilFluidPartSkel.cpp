//
// SNTNiteSilFluidPartSkel.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTNiteSilFluidPartSkel.h"

using namespace std;

namespace tav
{
    SNTNiteSilFluidPartSkel::SNTNiteSilFluidPartSkel(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs), inited(false), psInited(false), nrParticle(518400),
    flWidth(200), flHeight(200), emitPartPerUpdate(400000), thinDownSampleFact(8)
    {
    	kin = static_cast<KinectInput*>(scd->kin);
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        ps = new GLSLParticleSystemFbo(shCol, nrParticle,
        								float(scd->screenWidth) / float(scd->screenHeight));
        
//        ps = new GLSLParticleSystem2(shCol, nrParticle,
//                                    scd->screenWidth, scd->screenHeight);
        ps->setFriction(0.1f);
        ps->setLifeTime(4.f);
        ps->setAging(false);
        ps->setAgeFading(false);
//        ps->init();

        
        fluidSim = new GLSLFluid(false, shCol);
        fluidSim->allocate(flWidth, flHeight, 0.5f);
        
        fluidSim->dissipation = 0.999f;
        fluidSim->velocityDissipation = 0.99f;
        fluidSim->setGravity(glm::vec2(0.0f, 0.0f));

        forceScale = glm::vec2(static_cast<float>(flWidth) * 0.25f,
                               static_cast<float>(flHeight) * 0.25f);

        edgeDetect = shCol->addCheckShader("edgeDetect", "shaders/edgeDetect.vert", "shaders/edgeDetect.frag");
        edgeDetect->link();
        
        nis = kin->getNis();

        nrForceJoints = 15;
        oldJointPos = new glm::vec2*[maxNrUsers];
        for(int i=0;i<maxNrUsers;i++)
            oldJointPos[i] = new glm::vec2[nrForceJoints];
        
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
        
        userMapTuttiTex = new TextureManager();
        userMapTex = new TextureManager*[maxNrUsers];
        for(int i=0;i<maxNrUsers;i++)
            userMapTex[i] = new TextureManager();
        
        kinColorTex = new TextureManager*[2];
        for(short i=0;i<2;i++)
            kinColorTex[i] = new TextureManager();

        rawQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
                           glm::vec3(0.f, 0.f, 1.f),
                           0.f, 0.f, 0.f, 0.f);    // color will be replace when rendering with blending on

        colShader = shCol->getStdCol();
        texShader = shCol->getStdTex();
    }
    
    //---------------------------------------------------------------
    
    SNTNiteSilFluidPartSkel::~SNTNiteSilFluidPartSkel()
    {
        delete ps;
        delete fluidSim;
    }
    
    //---------------------------------------------------------------
    
    void SNTNiteSilFluidPartSkel::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo)
        {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_DEPTH_BOUNDS_TEST_EXT);     // is needed for drawing cubes
        glDisable(GL_CULL_FACE);     // is needed for drawing cubes
        glDisable(GL_STENCIL_TEST);     // is needed for drawing cubes

//        fluidSim->draw();
//        fluidSim->drawVelocity();
        
//        ps->bindStdShader(GLSLParticleSystem::QUADS);
//        for (auto i=0;i<MAX_NUM_SIM_TEXS;i++)
//        {
//            glActiveTexture(GL_TEXTURE0+i);
//            glBindTexture(GL_TEXTURE_2D, taMods->textures[i]);
//        }

        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//        ps->draw(cp->mvp_mat4);
        
        
        texShader->begin();
        texShader->setIdentMatrix4fv("m_pvm");
        texShader->setUniform1i("tex", 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, userMapTex[0]->getId());
//        glBindTexture(GL_TEXTURE_2D, userMapTuttiTex->getId());
        rawQuad->draw();
        
        // restart the actual light shader
        //        _shader->begin();
        //        sendStdShaderInit(_shader);
    }
    
    //---------------------------------------------------------------
    
    
    void SNTNiteSilFluidPartSkel::update(double time, double dt)
    {
        if(!inited)
        {
            userMapTuttiTex->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
                                      GL_R8, GL_RED, GL_TEXTURE_2D, GL_UNSIGNED_BYTE); // for the userMap
            
            for(short i=0;i<maxNrUsers;i++)
                userMapTex[i]->allocate(kin->getDepthWidth() / thinDownSampleFact,
                                        kin->getDepthHeight() / thinDownSampleFact,
                                        GL_R8, GL_RED, GL_TEXTURE_2D, GL_UNSIGNED_BYTE); // for the userMap
            
            for(short i=0;i<2;i++)
                kinColorTex[i]->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
                                         GL_RGB8, GL_RGB, GL_TEXTURE_2D, GL_UNSIGNED_BYTE); // for the userMap

            optFlow = new GLSLOpticalFlow(shCol, kin->getDepthWidth(),
                                          kin->getDepthHeight());

            userMapConvTutti = new uint8_t[kin->getDepthWidth() * kin->getDepthHeight()];
            
            userMapConv = new uint8_t*[maxNrUsers];
            for (int i=0;i<maxNrUsers;i++)
            {
                userMapConv[i] = new uint8_t[kin->getDepthWidth() * kin->getDepthHeight() / thinDownSampleFact];
                memset(userMapConv[i], 0, kin->getDepthWidth() * kin->getDepthHeight() / thinDownSampleFact);
            }

            // mach pro zeile einen vector
            for(int y=0;y<kin->getDepthHeight() / thinDownSampleFact;y++)
                userThinned.push_back( vector<short>() );
            
            edgePP = new PingPongFbo(shCol, kin->getDepthWidth(),
                                     kin->getDepthHeight(), GL_RGB8, GL_TEXTURE_2D);
            
            optFlow = new GLSLOpticalFlow(shCol, kin->getDepthWidth(),
                                          kin->getDepthHeight());
            optFlow->setMedian(10.f);

            data.emitOrg = glm::vec3(0.f, 0.f, 0.f);
            data.posRand = 1.f;
            data.emitVel = glm::vec3(1.f, 0.f, 0.f);
            data.speed = 0.0f;
            data.size = 0.06f;
            data.texUnit = 1;
            data.texRand = 1.f;
            data.angleRand = 1.f;
            data.colInd = 1.f;
            data.emitCol = glm::vec4(1.f, 1.f, 1.f, 1.f);
            
            //ps->emit(emitPartPerUpdate, data);

            inited = true;
        }
        
        // add users
        if (kin->isNisInited() && inited)
        {
            // get movement
            if (colFrameNr != kin->getColFrameNr())
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
                
                optFlow->update(kinColorTex[kinColorImgPtr]->getId(),
                				kinColorTex[(kinColorImgPtr +1) % 2]->getId());
            }
            
            // get silhoutte
            if(frameNr != kin->getNisFrameNr())
            {
                frameNr = kin->getNisFrameNr();
                updateSil();
            }
            
            fluidSim->update();
            
            massCenter = glm::vec3(1.f, 0.f, 0.f);

            // emit particles with emit texture
//            ps->emit(time, emitPartPerUpdate, data, false, edgePP->getSrcTexId(),
//                     kin->getDepthWidth(), kin->getDepthHeight(), &massCenter);
        }
        
        //mouseTest(time);
        
        // update particle system
        //ps->update(time);
        ps->update(time, fluidSim->getVelocityTex());
    }
    
    //---------------------------------------------------------------
    
    void SNTNiteSilFluidPartSkel::updateSil()
    {
        // get joints for velocity
        const nite::Array<nite::UserData>& users = nis->userTrackerFrame.getUsers();
        const nite::UserMap& userLabels = nis->getUserMap();
        const nite::UserId* pLabels = userLabels.getPixels();
        
        procUserMaps(userMapConvTutti, userMapTuttiTex->getId(), pLabels);

        // ----

        // debug: upload thinned texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, userMapTex[0]->getId());
        glTexSubImage2D(GL_TEXTURE_2D,             // target
                        0,                          // First mipmap level
                        0, 0,                       // x and y offset
                        kin->getDepthWidth() / thinDownSampleFact,
                        kin->getDepthHeight() / thinDownSampleFact,
                        GL_RED,
                        GL_UNSIGNED_BYTE,
                        userMapConv[0]);
        
        // ----
        
        
        // make maxNrUser Textures for thinning
//        for (int usrNr=0; usrNr<maxNrUsers; usrNr++)
//            uploadUserMaps(userMapConv[usrNr], userMapTex[usrNr]->getId(), pLabels, usrNr);
        
        // -- convert nite silhoutte to edge --
        
        edgePP->dst->bind();
        edgePP->dst->clear();
        
        edgeDetect->begin();
        edgeDetect->setIdentMatrix4fv("m_pvm");
        edgeDetect->setUniform1f("stepX", 1.f / static_cast<float>(scd->screenWidth));
        edgeDetect->setUniform1f("stepY", 1.f / static_cast<float>(scd->screenHeight));
        edgeDetect->setUniform1i("tex", 0);
            
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, userMapTuttiTex->getId());
        
        rawQuad->draw();
        
        edgePP->dst->unbind();
        edgePP->swap();
        
        /*
        //  velocity 
        
        for (int i=0; i<users.getSize(); ++i)
        {
            const nite::UserData& user = users[i];
            
            if (user.isVisible())
            {
                for (int jNr=0;jNr<nrForceJoints;jNr++)
                {
                    glm::vec2 m = glm::vec2((nis->skelData->getJoint(i, forceJoints[jNr]).x * 0.5f + 0.5f) * flWidth,
                                            (1.f - (nis->skelData->getJoint(i, forceJoints[jNr]).y * 0.5f + 0.5f)) * flHeight);
                    glm::vec2 d = (m - oldJointPos[i][jNr]) * 1.f;
                    glm::vec2 c = glm::normalize(forceScale - m);

                    if (jNr == 6)
                    {
                        std::cout << "m: " << glm::to_string(m) << " d:" << glm::to_string(d) << std::endl;
                    }

                    //std::cout << "[" << i << "]" << glm::to_string(m) << ", " << glm::to_string(d) << std::endl;
                    
                    fluidSim->addTemporalVelocity(m, d, 3.0f);
                    //                        fluidSim->addTemporalForce(m, d, eCol, 2.0f);
                    
                    oldJointPos[i][jNr] = m;
                }
            }
        }
        
        glm::vec3 multCol = glm::vec3(1.f, 0.56f, 0.08f);
        glm::vec3 eCol = glm::vec3(25.f / 255.f,
                                   39.f / 255.f,
                                   175.f / 255.f);
        
        fluidSim->addColor(edgePP->getSrcTexId(), eCol, taMods->oscData->blurFboAlpha * 2.f);
         */
    }
    
    
    
    void SNTNiteSilFluidPartSkel::mouseTest(double time)
    {
        // mouse add force to debug
        // Adding temporal Force, in pixel relative to flWidth and flHeight
        glm::vec2 m = glm::vec2(mouseX * static_cast<float>(fluidSim->getWidth()),
                                mouseY * static_cast<float>(fluidSim->getHeight()));
        glm::vec2 d = (m - oldM) * 5.f;
        oldM = m;
        glm::vec2 c = glm::normalize(forceScale - m);
        
        glm::vec4 eCol = glm::vec4(25.f / 255.f,
                                   39.f / 255.f,
                                   175.f / 255.f,
                                   0.01f);
        
        float tm = static_cast<float>(std::sin(time));
        
        fluidSim->addTemporalForce(m,                           // pos
                                   d,                           // vel
                                   eCol,
                                   2.0f);
        fluidSim->update();
    }

    
    
    // geh durch das bild durch und mach ein schmutzige thinnig
    // berechne immer die mitte jeder horizontalen linie
    // wenn zwischen zwei benachbarten horizontalen Pixeln ein unterschied
    // besteht fang zu zählen an. wenn wieder unterschied, hör auf.
    // gehe aber nicht durch jedes pixel sondern, nur pro zeile % factor
    // momentan werden alle user in ein array geschrieben
    // für reines velocity berechnen, ist es egal...
    
    void SNTNiteSilFluidPartSkel::procUserMaps(uint8_t* _map, GLint tex, const nite::UserId* pLabels)
    {
        float factor = 1;
        uint8_t* pTexRow = _map;
        int xPosStart = 0;
        int xPosCounter = 0;
        int actX, midPoint;
        short actCountUser = 0;
        
        // debug clear contr img
        for (short usrNr=0;usrNr<maxNrUsers;usrNr++)
            memset(userMapConv[usrNr], 0, kin->getDepthWidth() * kin->getDepthHeight() / thinDownSampleFact);
        
        // clear found points
        userThinnedPoints.clear();
        
        for (int y=0; y < kin->getDepthHeight() ; ++y)
        {
            uint8_t* pTex = pTexRow; // get the pointer to the first pixel in the actual row
            
            // pro zeile
            xPosStart = -1;
            xPosCounter = -1;
            actCountUser = -1;

            // beginning of line, reset the per line counter
            if((y % thinDownSampleFact) == 0)
                userThinned[y / thinDownSampleFact].clear();
            
            for (int x=0; x < kin->getDepthWidth() ; ++x, ++pLabels)
            {
                // downsampling, check this pixel
                if ((x > 0 && y > 0) && (x % thinDownSampleFact) == 0 && (y % thinDownSampleFact) == 0)
                {
                    if ( *pLabels != 0 )
                    {
                        factor = 255.f;
                    } else {
                        factor = 0.f;
                    }
                                        
                    const nite::UserId* leftNeighbour = pLabels;
                    leftNeighbour -= thinDownSampleFact;
                    
                    // if neighbour different start counting
                    if (*pLabels != *leftNeighbour)
                    {
                        // wenn counting was started, save the actual midpoints
                        if (xPosStart != -1 && actCountUser >= 0)
                        {
                            midPoint = (x - xPosStart) / 2 + xPosStart;
                            userThinned[y / thinDownSampleFact].push_back((x - xPosStart) / 2 + xPosStart);
                            //userMapConv[0][((y * (kin->getDepthWidth() / thinDownSampleFact)) + midPoint) / thinDownSampleFact] = 255.f;
                        }
                        
                        if ((*pLabels) != 0)
                        {
                            xPosStart = x;
                            xPosCounter = 0;
                            actCountUser = (*pLabels) -1;

                        } else
                        {
                            xPosStart = -1;
                            xPosCounter = -1;
                            actCountUser = -1;
                        }
                    } else
                    {
                        xPosCounter++;
                    }
                }
                
                if ( *pLabels != 0 )
                {
                    factor = 255.f;
                } else {
                    factor = 0.f;
                }
                
                *(pTex++) = factor;
            }
            
            // end of line, check if the number of found points change to the last line
            // if this is the case, add all points in this line
            if(y > 0 && (y % thinDownSampleFact) == 0 && (int)userThinned[y / thinDownSampleFact].size() > 0)
            {
                if((int)userThinned[y / thinDownSampleFact].size() != (int)userThinned[(y - thinDownSampleFact) / thinDownSampleFact].size())
                {
                    std::vector<short>::iterator it = userThinned[y / thinDownSampleFact].begin();
                    for (;it!= userThinned[y / thinDownSampleFact].end();++it)
                    {
                        userThinnedPoints.push_back( thinPoint() );
                        userThinnedPoints.back().pos = glm::vec2((float)(*it), (float)y);
                        userMapConv[0][((y * (kin->getDepthWidth() / thinDownSampleFact)) + (*it)) / thinDownSampleFact] = 255.f;
                    }
                }
            }

            
            pTexRow += kin->getDepthWidth();
        }
        
        // upload
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexSubImage2D(GL_TEXTURE_2D,             // target
                        0,                          // First mipmap level
                        0, 0,                       // x and y offset
                        kin->getDepthWidth(),
                        kin->getDepthHeight(),
                        GL_RED,
                        GL_UNSIGNED_BYTE,
                        _map);
    }

    
    
    void SNTNiteSilFluidPartSkel::onKey(int key, int scancode, int action, int mods)
    {}
    
    
    
    void SNTNiteSilFluidPartSkel::onCursor(GLFWwindow* window, double xpos, double ypos)
    {
        mouseX = xpos / scd->screenWidth;
        mouseY = (ypos / scd->screenHeight);
    }
    
}
