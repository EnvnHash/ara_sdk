//
// SNTNiteSilFluidPart.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  braucht externen shader mit texturen zum rendern
//
// capture copy to /media/ingeneria1 192.168.0.42, diese kiste .43

#include "SNTNiteSilFluidPart.h"

using namespace std;

namespace tav
{
    SNTNiteSilFluidPart::SNTNiteSilFluidPart(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs), psInited(false), nrParticle(435600),
    flWidth(200), flHeight(200), emitPartPerUpdate(3000), thinDownSampleFact(8),
    fblurSize(512),
    partTexScale(1.f), noiseTexSize(1024), renderMode(0), nrCameras(1),
    captureIntv(120.0), depthTexMedian(0.4f),
    niteResetIntv(1800.0), // reset ni anyway after this time (in sec) each half an hour
    inActNiResetInt(1200.0),   // use the opportunity reset in case of inactivity after of 10 min
    inActAddNoiseInt(40.0)  // wenn nix los emit noise
    {
    	kin = static_cast<KinectInput*>(scd->kin);
    	osc = static_cast<OSCData*>(scd->osc);
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        nis = kin->getNis();

        // - Partikel System --
        
        ps = new GLSLParticleSystemFbo(shCol, nrParticle,
                                       float(scd->screenWidth) / float(scd->screenHeight));
        ps->setFriction(0.1f);
        ps->setLifeTime(3.f);
        ps->setAging(true);
        ps->setAgeFading(true);
        data.emitOrg = glm::vec3(0.f, 0.f, 0.f);
        data.emitVel = glm::vec3(1.f, 0.f, 0.f);
        data.size = 0.06f;
        data.texUnit = 1;
        data.texRand = 1.f;
        data.angleRand = 1.f;
        data.colInd = 1.f;
        data.emitCol = glm::vec4(1.f, 1.f, 1.f, 1.f);
        data.colRand = 0.85f;
        data.posRand = 0.f;
        data.speed = 0.f;
        ps->setEmitData(&data);
        
        renderPartPP = new PingPongFbo*[nrCameras];
        for (short i=0;i<nrCameras;i++)
            renderPartPP[i] = new PingPongFbo(shCol,
                                              static_cast<int>((float)scd->screenWidth * partTexScale),
                                              static_cast<int>((float)scd->screenHeight * partTexScale),
                                              GL_RGBA8, GL_TEXTURE_2D);
        
        fluidAddCol = new glm::vec4[3];
        fluidAddCol[0] = glm::vec4(25.f / 255.f,
                                   39.f / 255.f,
                                   175.f / 255.f,
                                   1.f);
        fluidAddCol[1] = glm::vec4(247.f / 255.f,
                                   159.f / 255.f,
                                   35.f / 255.f,
                                   1.f);
        fluidAddCol[2] = glm::vec4(134.f / 255.f,
                                   26.f / 255.f,
                                   255.f / 255.f,
                                   1.f);
        
        partEmitCol = new glm::vec4[3];
        partEmitCol[0] = glm::vec4(182.f / 255.f,
                                   189.f / 255.f,
                                   251.f / 255.f,
                                   1.f);
        partEmitCol[1] = glm::vec4(181.f / 255.f,
                                   248.f / 255.f,
                                   227.f / 255.f,
                                   1.f);
        partEmitCol[2] = glm::vec4(248.f / 255.f,
                                   241.f / 255.f,
                                   181.f / 255.f,
                                   1.f);

        // - Fluid System --
        
        fluidSim = new GLSLFluid(false, shCol);
        fluidSim->allocate(flWidth, flHeight, 0.5f);
        
        fluidSim->dissipation = 0.999f;
        fluidSim->velocityDissipation = 0.999f;
        fluidSim->setGravity(glm::vec2(0.0f, 0.0f));
        
        forceScale = glm::vec2(static_cast<float>(flWidth) * 0.25f,
                               static_cast<float>(flHeight) * 0.25f);
        
        
        // --- Textures ---
        
        userMapTex = new TextureManager();
        tempDepthTex = new TextureManager();
        kinColorTex = new TextureManager*[2];
        for(short i=0;i<2;i++)
            kinColorTex[i] = new TextureManager();

        
        // --- Geo Primitives ---
        
        rawQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
                           glm::vec3(0.f, 0.f, 1.f),
                           0.f, 0.f, 0.f, 0.f);    // color will be replace when rendering with blending on
        
        rotateQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
                              glm::vec3(0.f, 0.f, 1.f),
                              0.f, 0.f, 0.f, 0.f,
                              nullptr, 1, true);
        
        
        // --- Shaders ---
        
        colShader = shCol->getStdCol();
        texShader = shCol->getStdTex();
        blendTexShader = shCol->addCheckShader("texBlendShdr", "shaders/basic_tex.vert", "shaders/basic_tex_alpha.frag");
        noiseShader = shCol->addCheckShader("perlinVec3", "shaders/perlin.vert", "shaders/perlinVec3.frag");
        xBlendShaderH = shCol->addCheckShader("xBlendH", "shaders/basic_tex.vert", "shaders/xBlendH.frag");
        xBlendShaderV = shCol->addCheckShader("xBlendV", "shaders/basic_tex.vert", "shaders/xBlendV.frag");
        mappingShaderTex = shCol->addCheckShader("cam3ScalerTex", "shaders/cam3scalerTex.vert", "shaders/cam3scalerTex.geom",  "shaders/cam3scalerTex.frag");
        edgeDetect = shCol->addCheckShader("edgeDetect", "shaders/edgeDetect.vert", "shaders/edgeDetect.frag");
        
        //  screen / mapping setup 
        
        leftScreenDim = glm::vec2(480.f, 480.f);
        centerScreenDim = glm::vec2(600.f, 480.f);
        rightScreenDim = glm::vec2(480.f, 480.f);
        totScreenDim = glm::vec2(leftScreenDim.x + centerScreenDim.x + rightScreenDim.x,
                                 leftScreenDim.y + centerScreenDim.y + rightScreenDim.y);

        screenYOffset = scd->screenHeight - leftScreenDim.y -rightScreenDim.y;
        
        vp = new glm::vec4[nrCameras];
        vp[0] = glm::vec4(0.f,              rightScreenDim.y +screenYOffset,  leftScreenDim.x, leftScreenDim.y);      // left top
        if(nrCameras > 1)
        {
            vp[1] = glm::vec4(rightScreenDim.x, rightScreenDim.y +screenYOffset,  centerScreenDim.x, centerScreenDim.y);  // right top
            vp[2] = glm::vec4(0.f,              screenYOffset,                    rightScreenDim.x, rightScreenDim.y);    // left bottom
        }
        
        
        texModMatrices = new glm::mat4[nrCameras];
        pointModMatrices = new glm::mat4[nrCameras];
        multCamMatr = new glm::mat4[nrCameras];
        projMatrices = glm::mat4(1.f);
        
        for (short i=0;i<nrCameras;i++)
        {
            texModMatrices[i] = glm::mat4(1.f);
            pointModMatrices[i] = glm::mat4(1.f);
            multCamMatr[i] = glm::mat4(1.f);
        }

        // skaliere die texturen auf die gesamte breite der leinwände, proportional
        // zu ihrer grösse
        texModMatrices[0] = glm::scale(texModMatrices[0], glm::vec3(totScreenDim.x / leftScreenDim.x * 2.f, 1.f, 1.f));
        texModMatrices[0] = glm::translate(texModMatrices[0], glm::vec3((centerScreenDim.x * 0.5f +
                                                                         leftScreenDim.x * 0.5f) / totScreenDim.x,
                                                                        0.f, 0.f));
        if (nrCameras > 1)
        {
            texModMatrices[1] = glm::scale(texModMatrices[1], glm::vec3(totScreenDim.x / centerScreenDim.x * 2.f, 1.f, 1.f));
         
            texModMatrices[2] = glm::scale(texModMatrices[2], glm::vec3(totScreenDim.x / rightScreenDim.x * 2.f, 1.f, 1.f));
            texModMatrices[2] = glm::translate(texModMatrices[2], glm::vec3((centerScreenDim.x * 0.5f +
                                                                             rightScreenDim.x * 0.5f) / totScreenDim.x * -1.f,
                                                                            0.f, 0.f));
        }
        
        // matrize für die linke seite, dreh alles um die y-Achse um 90grad im gegenuhrzeigersinn
        pointModMatrices[0] = glm::rotate(pointModMatrices[0], static_cast<float>(M_PI * -0.5), glm::vec3(0.f, 1.f, 0.f));
        
        // matrize für die rechte seite, dreh alles um die y-Achse um 90grad im gegenuhrzeigersinn
        if (nrCameras > 1)
            pointModMatrices[2] = glm::rotate(pointModMatrices[2], static_cast<float>(M_PI * 0.5), glm::vec3(0.f, 1.f, 0.f));
        
        identMatr = glm::mat4(1.f);

        // --- standard camera ---
        
        glm::vec3 lookAt = glm::vec3(0.f, 0.f, 0.f);
        glm::vec3 upVec = glm::vec3(0.f, 1.f, 0.f);
        stdCam = new GLMCamera(GLMCamera::FRUSTUM,
                               scd->screenWidth, scd->screenHeight,
                               -1.0f, 1.0f, -1.0f, 1.0f,       // left, right, bottom, top
                               0.f, 0.f, 1.f,                  // camPos
                               lookAt.x, lookAt.y, lookAt.z,   // lookAt
                               upVec.x, upVec.y, upVec.z,
                               1.f, 100.f);
        
        
        multCamMatr[0] = glm::mat4(1.f);
        multCamMatr[0] = glm::rotate(multCamMatr[0], static_cast<float>(M_PI * 0.5), glm::vec3(0.f, 1.f, 0.f));
        multCamMatr[0] = stdCam->getProjectionMatr() * stdCam->getViewMatr() * multCamMatr[0];
        
        if (nrCameras > 1)
        {
            multCamMatr[2] = glm::mat4(1.f);
            multCamMatr[2] = glm::rotate(multCamMatr[2], static_cast<float>(M_PI * -0.5), glm::vec3(0.f, 1.f, 0.f));
            multCamMatr[2] = stdCam->getProjectionMatr() * stdCam->getViewMatr() * multCamMatr[2];
        }
        
        
        // --- logo for upper right corner ---
        
        logoTex = new TextureManager();
        logoTex->loadTexture2D((*scd->dataPath)+"textures/House.jpg");
        
        float logoWidth = 0.3f; // means width of logo, 2.f is full
        float border = 0.05f;
        float logoAspectRatio = static_cast<float>(logoTex->getHeight()) / static_cast<float>(logoTex->getWidth());
        float screenAspectRatio = static_cast<float>(scd->screenHeight) / static_cast<float>(scd->screenWidth);
        float logoHeight = logoWidth * logoAspectRatio / screenAspectRatio;
        glm::vec2 logoLowerLeftCorner = glm::vec2(1.f - logoWidth - border, 1.f - logoHeight - border);
        
        // position to right upper corner for fullscreen
        logoQuadFullScreen = new Quad(logoLowerLeftCorner.x, logoLowerLeftCorner.y,
                                      logoWidth, logoHeight,
                                      glm::vec3(0.f, 0.f, 1.f),
                                      0.f, 0.f, 0.f, 0.f);
        
        // position to right upper corner for center screen
        screenAspectRatio = static_cast<float>(centerScreenDim.y) / static_cast<float>(centerScreenDim.x);
        logoHeight = logoWidth * logoAspectRatio / screenAspectRatio;
        logoLowerLeftCorner = glm::vec2(1.f - logoWidth, 1.f - logoHeight);
        logoQuadCenterScreen = new Quad(logoLowerLeftCorner.x, logoLowerLeftCorner.y,
                                        logoWidth, logoHeight,
                                        glm::vec3(0.f, 0.f, 1.f),
                                        0.f, 0.f, 0.f, 0.f);
        
        // -- capturing ----
        
        capturePic1Cam = cv::Mat(scd->screenHeight, scd->screenWidth, CV_8UC4);
        fCapturePic1Cam = cv::Mat(scd->screenHeight, scd->screenWidth, CV_8UC4);
        capturePic3Cam = cv::Mat((int)centerScreenDim.y, (int)centerScreenDim.x, CV_8UC4);
        fCapturePic3Cam = cv::Mat((int)centerScreenDim.y, (int)centerScreenDim.x, CV_8UC4);
    }
    
    
    
    void SNTNiteSilFluidPart::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo)
        {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // check if we need to capture a frame
        bool doCapture = (time - lastCaptureTime) > captureIntv;
        doCapture = false;
        
        // render a plain 1 camera setup
        if (renderMode == 0)
        {
           // printf("draw \n");

            texShader->begin();
            texShader->setIdentMatrix4fv("m_pvm");
            texShader->setUniform1i("tex", 0);
            glActiveTexture(GL_TEXTURE0);
            //            glBindTexture(GL_TEXTURE_2D, fluidSim->getVelocityTex());
            glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());
            rotateQuad->draw();
            
            //fluidSim->drawVelocity();
            
            
            // draw rendered particles
            texShader->begin();
            texShader->setIdentMatrix4fv("m_pvm");
            texShader->setUniform1i("tex", 0);
            glBindTexture(GL_TEXTURE_2D, renderPartPP[0]->getSrcTexId());
            rawQuad->draw();
            
            
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            
            // draw raw silhouette
            blendTexShader->begin();
            blendTexShader->setIdentMatrix4fv("m_pvm");
            blendTexShader->setUniform1i("tex", 0);
            blendTexShader->setUniform1f("alpha", osc->blurFboAlpha);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fblur->getResult());
            rotateQuad->draw();
            
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // draw logo
//            glBindTexture(GL_TEXTURE_2D, logoTex->getId());
//            logoQuadFullScreen->draw();
            
            //if (doCapture) startCaptureFrame(renderMode, time);
            
        } else
        {
            glDisable(GL_DEPTH_TEST);
            glScissor(0, 0, scd->screenWidth, scd->screenHeight);
            texShader->begin();
            texShader->setUniform1i("tex", 0);
            
            for (short i=0;i<nrCameras;i++)
            {
                glViewportIndexedf(0, vp[i].x, vp[i].y, vp[i].z, vp[i].w);      // left top
                texShader->setUniformMatrix4fv("m_pvm", &texModMatrices[i][0][0]);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());
                rotateQuad->draw();
            }
    
            // draw particle ppFbos
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            texShader->begin();
            texShader->setIdentMatrix4fv("m_pvm");
            texShader->setUniform1i("tex", 0);
            glActiveTexture(GL_TEXTURE0);
            
            // render the three rendered particles texture
            for (short i=0;i<nrCameras;i++)
            {
                glScissor(vp[i].x, vp[i].y, vp[i].z, vp[i].w);
                glViewportIndexedf(0, vp[i].x, vp[i].y, vp[i].z, vp[i].w);
                glBindTexture(GL_TEXTURE_2D, renderPartPP[i]->getSrcTexId());
                rawQuad->draw();
                
                // draw logo in the center
                // capture center screen if needed
                if (i==1)
                {
                    glBindTexture(GL_TEXTURE_2D, logoTex->getId());
                    logoQuadCenterScreen->draw();
                    
                    if (doCapture) startCaptureFrame(renderMode, time);
                }
            }
            
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            
            // draw raw silhouette
            glViewportIndexedf(0, vp[1].x, vp[1].y, vp[1].z, vp[1].w);      // left top
            glScissor(vp[1].x, vp[1].y, vp[1].z, vp[1].w);
            
            blendTexShader->begin();
            blendTexShader->setIdentMatrix4fv("m_pvm");
            blendTexShader->setUniform1i("tex", 0);
            blendTexShader->setUniform1f("alpha", osc->blurFboAlpha);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fblur->getResult());
            rotateQuad->draw();
        }
        /*
        glScissor(vp[1].x, vp[1].y, vp[1].z, vp[1].w);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewportIndexedf(0, vp[1].x, vp[1].y, vp[1].z, vp[1].w);

         texShader->begin();
         texShader->setIdentMatrix4fv("m_pvm");
         texShader->setUniform1i("tex", 0);
         glActiveTexture(GL_TEXTURE0);
         //        glBindTexture(GL_TEXTURE_2D, kinColorTex[kinColorImgPtr]->getId());
         //        glBindTexture(GL_TEXTURE_2D, tempDepthTex->getId());
         glBindTexture(GL_TEXTURE_2D, fblur->getResult());
         //        glBindTexture(GL_TEXTURE_2D, optFlow->getResTexId());
         //glBindTexture(GL_TEXTURE_2D, edgePP->getSrcTexId());
         rawQuad->draw();
         */
    }
    
    
    
    void SNTNiteSilFluidPart::set3CamViewPorts()
    {
        for (short i=0;i<nrCameras;i++)
            glViewportIndexedf(i, vp[i].x, vp[i].y, vp[i].z, vp[i].w);      // left top
    }
    
    
    
    void SNTNiteSilFluidPart::update(double time, double dt)
    {
        if(!inited)
        {
            userMapConv = new uint8_t[kin->getDepthWidth() * kin->getDepthHeight()];
            memset(userMapConv, 0, kin->getDepthWidth() * kin->getDepthHeight());
            userMapTex->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
                                 GL_R8, GL_RED, GL_TEXTURE_2D, GL_UNSIGNED_BYTE); // for the userMap
            
            userMapRGBA = new uint8_t[kin->getDepthWidth() * kin->getDepthHeight() * 4];
            memset(userMapRGBA, 0, kin->getDepthWidth() * kin->getDepthHeight() * 4);
            for(short i=0;i<2;i++)
                kinColorTex[i]->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
                                         GL_RGBA8, GL_RGBA, GL_TEXTURE_2D, GL_UNSIGNED_BYTE); // for the userMap
            tempDepthTex->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
                                   GL_RGBA8, GL_RGBA, GL_TEXTURE_2D, GL_UNSIGNED_BYTE);
            
            edgePP = new PingPongFbo(shCol, kin->getDepthWidth(),
                                     kin->getDepthHeight(), GL_RGB8, GL_TEXTURE_2D);
            
            optFlow = new GLSLOpticalFlow(shCol, kin->getDepthWidth(),
                                          kin->getDepthHeight());
            optFlow->setMedian(0.8f);
            
            // generate perlinNoise Texture for Windsimulation
            perlinNoise = new FBO(shCol, noiseTexSize, noiseTexSize);
            xBlendFboH = new FBO(shCol, noiseTexSize, noiseTexSize);
            xBlendFboV = new FBO(shCol, noiseTexSize, noiseTexSize);
            
            genWindTex();
            
            fblur = new FastBlurMem(0.06f, shCol, fblurSize, fblurSize);
            

            
            //ps->emit(emitPartPerUpdate, data);
            
            inited = true;
        }
        
        fblur->setAlpha(0.18f);
//        fblur->setAlpha(osc->blurOffs);

        // osc tuninng
        // particle feedback
        ps->setLifeTime(4.f);
        //depthTexMedian = osc->rotYAxis;
        
        // check if Nite-Reset is needed
        if(inActCounter > inActNiResetInt || (time - lastNiteReset) > niteResetIntv)
        {
            printf("reset nite \n");
            nis->reset();
            lastNiteReset = time;
            inActCounter = 0;
        }
        
        // wenn nix los, emit particles
        if(inActEmitTime > inActAddNoiseInt)
        {
            inactivityEmit = true;
        } else {
            inactivityEmit = false;
        }
        
        
        // add users
        if (kin->isNisInited() && inited)
        {
            // get new depth frame
            if(frameNr != kin->getNisFrameNr())
            {
                frameNr = kin->getNisFrameNr();

                // download depth frame from kinect
                updateSil(dt);
                
                optFlow->update(fblur->getResult(), fblur->getLastResult());
                
                fluidSim->addVelocity(optFlow->getResTexId(), 2.f);
//                fluidSim->addVelocity(optFlow->getResTexId(), osc->alpha * 4.f);
                
                float timeMult = time * 1.2f;
                float colPos = static_cast<float>(std::sin(timeMult * (std::sin(timeMult * 0.01) * 0.5 + 0.5) * 0.2)) * 0.5f + 0.5f;
                glm::vec4 eCol = std::fmax(1.f - colPos *2.f, 0.f) * fluidAddCol[0]
                + std::fmax(1.f - std::abs(colPos *2.f -1.f), 0.f) *fluidAddCol[1]
                + std::fmax((colPos -0.5f) *2.f, 0.f) *fluidAddCol[2];
                
                fluidSim->addColor(edgePP->getSrcTexId(), glm::vec3(eCol), 2.f);
            }
            
//            fluidSim->draw();
            fluidSim->update();
            
            // emit Color
            float peclTime = osc->videoSpeed * time;
            float colPos2 = static_cast<float>(std::sin(peclTime * (std::sin(peclTime * 0.01) * 0.5 + 0.5) * 0.2)\
                                               * std::sin(peclTime * 0.02)) * 0.5f + 0.5f;
            data.emitCol = std::fmax(1.f - colPos2 *2.f, 0.f) * partEmitCol[0]
            + std::fmax(1.f - std::abs(colPos2 *2.f -1.f), 0.f) *partEmitCol[1]
            + std::fmax((colPos2 -0.5f) *2.f, 0.f) *partEmitCol[2];
            
//            data.emitCol *= osc->totalBrightness;
            data.emitCol.a = 0.7f;
            
            
            // emit particles with emit texture and depth info
            //emitTexCounter++;
            
            //            if (emitTexCounter > static_cast<int>(osc->blurFboAlpha * 20.f))
            //{
            ps->emit(emitPartPerUpdate,
                     edgePP->getSrcTexId(), kin->getDepthWidth(), kin->getDepthHeight());
//                     fblur->getResult(), fblurSize, fblurSize);
            //emitTexCounter = 0;
            //}
            
        }

		if (inactivityEmit)
		{
			data.posRand = (std::sin(time * 1.24) * 0.5f + 0.5f) * 0.2f;
			data.emitVel = glm::vec3(std::sin(time * 1.23), std::sin(time * 1.93),
					std::sin(time * 1.33));
			data.speed = 0.6f;

			data.emitOrg = glm::vec3(std::sin(time * 1.13) * std::sin(time * 0.3),
					std::sin(time * 0.612) * std::sin(time * 1.32),
					std::sin(time * 0.812) * std::sin(time * 0.822) * 0.5f - 0.5f);

            ps->emit(static_cast<int>(std::fmax(std::sin(time * 2.83f) * std::sin(time * 1.23f)
                                                * std::sin(time * 0.783f), 0.f) * 50.f));
		}

        // update particle system
        ps->update(time, fluidSim->getVelocityTex(), xBlendFboV->getColorImg());
        
        renderPartFbo();
    }
    
    
    
    void SNTNiteSilFluidPart::updateSil(double dt)
    {
        procUserMaps(dt);
        
        // -- convert nite silhoutte to edge --
        
        edgePP->dst->bind();
        edgePP->dst->clear();
        
        edgeDetect->begin();
        edgeDetect->setIdentMatrix4fv("m_pvm");
        edgeDetect->setUniform1f("stepX", 1.f / static_cast<float>(scd->screenWidth));
        edgeDetect->setUniform1f("stepY", 1.f / static_cast<float>(scd->screenHeight));
        edgeDetect->setUniform1i("tex", 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, userMapTex->getId());
        
        rawQuad->draw();
        
        edgePP->dst->unbind();
        edgePP->swap();
    }
    
    
    
    void SNTNiteSilFluidPart::mouseTest(double time)
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
    
    //-- can be optimized, im moment werde immer alle pixel geschrieben... -
    //-- kann auch threaded laufen, ist aber fraglich, ob so viel schneller... ---
    
    void SNTNiteSilFluidPart::procUserMaps(double dt)
    {
        if (nis->userTrackerFrame.isValid())
        {
            const nite::Array<nite::UserData>& users = nis->userTrackerFrame.getUsers();
            const nite::UserMap& userLabels = nis->getUserMap();
            const nite::UserId* pLabels = userLabels.getPixels();
            float* pDepthRow = kin->getActDepthImg();
            int depthRowSize = kin->getDepthWidth();
            float factor = 1;
            
            // check for inactivity
            if ( users.getSize() == 0 )
            {
                inActCounter += dt;
                inActEmitTime += dt;
            } else {
                inActCounter = 0.0;
                inActEmitTime = 0.0;
            }
            
            uint8_t* pTexRow = userMapConv;
            uint8_t* pTexRGBARow = userMapRGBA;
            
            for (int y=0; y<kin->getDepthHeight(); ++y)
            {
                uint8_t* pTex = pTexRow; // get the pointer to the first pixel in the actual row
                uint8_t* pTexRGBA = pTexRGBARow; // get the pointer to the first pixel in the actual row
                float* pDepth = pDepthRow;
                
                for (int x=0; x<kin->getDepthWidth(); ++x, ++pLabels, ++pDepth)
                {
                    if ( *pLabels != 0 ) factor = 1.f; else factor = 0.f;
                    *(pTex++) = factor * 255.f;
                    
                    // set rgb values, pseudo color
                    for (short it=0;it<3;it++)
                    {
                        *(pTexRGBA) = ((*pDepth) * 0.00390625f * factor * depthTexMedian)
                        + *(pTexRGBA) * (1.f - depthTexMedian);
                        pTexRGBA++;
                    }
                    
                    // set alpha values
                    *(pTexRGBA++) = 255.f;
                }
                
                pTexRow += kin->getDepthWidth();
                pTexRGBARow += kin->getDepthWidth() *4;
                pDepthRow += depthRowSize;
            }
            
            
            // upload userMap for EdgeDetect
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
            
            
            kinColorImgPtr = ++kinColorImgPtr % 2;
            
            // upload DepthGray as userMapRGBA
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tempDepthTex->getId());
            //            glBindTexture(GL_TEXTURE_2D, kinColorTex[kinColorImgPtr]->getId());
            glTexSubImage2D(GL_TEXTURE_2D,             // target
                            0,                          // First mipmap level
                            0, 0,                       // x and y offset
                            kin->getDepthWidth(),
                            kin->getDepthHeight(),
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            userMapRGBA);
            
            fblur->proc(tempDepthTex->getId());
        }
    }
    
    
    void SNTNiteSilFluidPart::genWindTex()
    {
        glDisable(GL_DEPTH_TEST);
        
        // ---- generate perlin noise pattern ---
        
        perlinNoise->bind();
        perlinNoise->clear();
        
        noiseShader->begin();
        noiseShader->setIdentMatrix4fv("m_pvm");
        noiseShader->setUniform1f("width", (float)noiseTexSize);
        noiseShader->setUniform1f("height", (float)noiseTexSize);
        noiseShader->setUniform2f("noiseScale", 0.15f, 0.15f);
        rawQuad->draw();
        
        perlinNoise->unbind();
        
        // ---- blend texture perlin noise pattern to be used borderless ---
        // ---- blend horizontal ---
        
        xBlendFboH->bind();
        xBlendFboH->clear();
        
        xBlendShaderH->begin();
        xBlendShaderH->setIdentMatrix4fv("m_pvm");
        xBlendShaderH->setUniform1i("tex", 0);
        xBlendShaderH->setUniform1i("width", noiseTexSize);
        xBlendShaderH->setUniform1i("height", noiseTexSize);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, perlinNoise->getColorImg());
        
        rawQuad->draw();
        
        xBlendFboH->unbind();
        
        // ---- blend vertical ---
        
        xBlendFboV->bind();
        xBlendFboV->clear();
        
        xBlendShaderV->begin();
        xBlendShaderV->setIdentMatrix4fv("m_pvm");
        xBlendShaderV->setUniform1i("tex", 0);
        xBlendShaderV->setUniform1i("width", noiseTexSize);
        xBlendShaderV->setUniform1i("height", noiseTexSize);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, xBlendFboH->getColorImg());
        
        rawQuad->draw();
        
        xBlendFboV->unbind();
    }
    
    // -- bind particle PP-Buffer and draw -
    
    void SNTNiteSilFluidPart::renderPartFbo()
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        if (renderMode == 0)
        {
            renderPartPP[0]->dst->bind();
            
            // render old texture
            texShader->begin();
            texShader->setIdentMatrix4fv("m_pvm");
            texShader->setUniform1i("tex", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, renderPartPP[0]->getSrcTexId());
            rawQuad->draw();
            
            // clear with alpha
            renderPartPP[0]->dst->clearAlpha(0.7f);
//            renderPartPP[0]->dst->clearAlpha(osc->rotYAxis);
            //            renderPartPP[0]->dst->clearAlpha(osc->fee);
            
            // render new particles
            ps->draw(&identMatr[0][0]);
            
            renderPartPP[0]->dst->unbind();
            renderPartPP[0]->swap();
            
        } else if (renderMode == 1)
        {
            // render old texture
            for (short i=0;i<nrCameras;i++)
            {
                renderPartPP[i]->dst->bind();
                
                texShader->begin();
                texShader->setIdentMatrix4fv("m_pvm");
                texShader->setUniform1i("tex", 0);
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, renderPartPP[i]->getSrcTexId());
                rawQuad->draw();
                
                // clear with alpha
                renderPartPP[i]->dst->clearAlpha(osc->rotYAxis);
                //                renderPartPP[i]->dst->clearAlpha(osc->feedback);
                
                // render new particles
                //ps->draw(identMatr);
                //                ps->draw(&pointModMatrices[i][0][0]); // orthogonal
                ps->draw(&multCamMatr[i][0][0]);    // perspective
                
                renderPartPP[i]->dst->unbind();
                renderPartPP[i]->swap();
            }
        }
    }
    
    
    
    void SNTNiteSilFluidPart::startCaptureFrame(int renderMode, double time)
    {
        lastCaptureTime = time;
        
        // capture front buffer
        glReadBuffer(GL_FRONT);
        
        if(renderMode == 0)
            glReadPixels(0, 0, scd->screenWidth, scd->screenHeight, GL_BGRA, GL_UNSIGNED_BYTE, capturePic1Cam.data);
        else if(renderMode == 1)
            glReadPixels((int)vp[1].x, (int)vp[1].y, (int)vp[1].z, (int)vp[1].w, GL_BGRA, GL_UNSIGNED_BYTE, capturePic3Cam.data);
        
        // start write thread
        if(capture_Thread)
        {
            delete capture_Thread;
            capture_Thread = 0;
        }
        
        capture_Thread = new boost::thread(&SNTNiteSilFluidPart::captureFrame, this, renderMode, time);
    }
    
    
    
    void SNTNiteSilFluidPart::captureFrame(int renderMode, double time)
    {
        // start SaveThread
        char fileName [100];
        sprintf(fileName,
                ((*scd->dataPath)+"capture/lenovo%d%d%f.jpg").c_str(),
                static_cast<int>( ( static_cast<double>(savedNr) / 10.0 ) ),
                savedNr % 10,
                time);
        
        if(renderMode == 0)
        {
            cv::flip(capturePic1Cam, fCapturePic1Cam, 0);
            cv::imwrite(fileName, fCapturePic1Cam);
            
        } else if(renderMode == 1)
        {
            cv::flip(capturePic3Cam, fCapturePic3Cam, 0);
            cv::imwrite(fileName, fCapturePic3Cam);
        }
        
        // start scp thread
        if (!inactivityEmit)
        {
            int i = system(("cp "+std::string(fileName)+" /media/ingeneria1-pc").c_str());
            if (i != 0) printf ("Image Capture Error!!!! scp upload failed\n");
            
            printf("image written \n");
        }
    }

    
    
    void SNTNiteSilFluidPart::onKey(int key, int scancode, int action, int mods)
    {}
    
    
    
    void SNTNiteSilFluidPart::onCursor(GLFWwindow* window, double xpos, double ypos)
    {
        mouseX = xpos / scd->screenWidth;
        mouseY = (ypos / scd->screenHeight);
    }
    
    
    
    SNTNiteSilFluidPart::~SNTNiteSilFluidPart()
    {
        delete userMapConv;
        delete userMapRGBA;
        delete ps;
        delete fluidSim;
        delete fluidAddCol;
        delete partEmitCol;
        delete [] texModMatrices;
        delete [] pointModMatrices;
        delete [] vp;
        delete logoTex;
        delete userMapTex;
        delete tempDepthTex;
        delete [] kinColorTex;
        delete rawQuad;
        delete rotateQuad;
        delete vp;
        delete edgePP;
        delete optFlow;
    }
}
