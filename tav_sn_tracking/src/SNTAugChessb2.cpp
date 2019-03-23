//
// SNTAugChessb2.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  mirroring has to be off!

#include "SNTAugChessb2.h"

using namespace std;
using namespace cv;

namespace tav
{
    SNTAugChessb2::SNTAugChessb2(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs),
    actDrawMode(CHECK_KIN_OVERLAY),
    beamerThrowRatio(0.6f),
    kinFovY(48.6f),                     // gemessen 48,6, Handbuch sagt 45 Grad, onistream sagt 45,642
    beamerLowEdgeAngle(3.02f),          // wenn die linse des Beamers nicht zentriert ist, dieser wert bezieht sich auf 4:3
                                        // bei 16:10 anderer wert, alter wert 1,68
    getNrCheckSamples(20),              // anzahl von samples bis die werte als stabil angesehen werden
    invertCamBeamerMatr(true),
    beamerToObjIntv(15),
    realBoardWidth(24.3f),              // reale Breite des Schachbretts (alle kästchen)
    coordNormFact(0.1f)                 // zur besseren Handhabung der Kinect Realworld Koordinaten
    {
    	kin = static_cast<KinectInput*>(scd->kin);
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        kpCalib.camFovY = (kinFovY / 360.f) * 2.f * M_PI;   // gemessen, handbuch sagt 45 Grad, onistream sagt 45,642
        kpCalib.boardSize = cv::Size(8,4);
        kpCalib.beamModel = ULTRA_WIDE;
        kpCalib.beamerAspectRatio = 1.384f; // gemessen
//        kpCalib.beamerAspectRatio = static_cast<float>(scd->screenWidth) / static_cast<float>(scd->screenHeight);
        kpCalib.beamerThrowRatio = beamerThrowRatio;
        kpCalib.beamerLowEdgeAngle = (beamerLowEdgeAngle / 360.f) * 2.f * M_PI;
        kpCalib.invertMatr = invertCamBeamerMatr;
        kpCalib.nrSamples = getNrCheckSamples;
        
        testChessFile = cv::imread((*scd->dataPath)+"textures/chess_640.jpg");
        
        camBeamerMatrPath = (*scd->dataPath)+("calib_cam/xtion_to_benq.yml");
        

        quad = new Quad(-1.f, -1.f, 2.f, 2.f,
                        glm::vec3(0.f, 0.f, 1.f),
                        0.f, 0.f, 0.f, 1.f,
                        nullptr, 1, true);
        
        noFlipQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
                              glm::vec3(0.f, 0.f, 1.f),
                              0.f, 0.f, 0.f, 1.f);
        
        kinQuad = new Quad(-1.0f, .75f, 0.25f, 0.25f,
                           glm::vec3(0.f, 0.f, 1.f),
                           0.f, 0.f, 0.f, 1.f,
                           nullptr, 1, true);
        
        whiteQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
                             glm::vec3(0.f, 0.f, 1.f),
                             1.f, 1.f, 1.f, 1.f);
        
        xLine = new Line(2, 1.f, 0.f, 0.f, 1.f);
        GLfloat* xLinePoints = (GLfloat*) xLine->getMapBuffer(POSITION);
        xLinePoints[0] = -1.f; xLinePoints[1] = 0.f; xLinePoints[2] = 0.f;
        xLinePoints[3] = 1.f; xLinePoints[4] = 0.f; xLinePoints[5] = 0.f;
        xLine->unMapBuffer();
        
        yLine = new Line(2, 1.f, 0.f, 0.f, 1.f);
        GLfloat* yLinePoints = (GLfloat*) yLine->getMapBuffer(POSITION);
        yLinePoints[0] = 0.f; yLinePoints[1] = 1.f; yLinePoints[2] = 0.f;
        yLinePoints[3] = 0.f; yLinePoints[4] = -1.f; yLinePoints[5] = 0.f;
        yLine->unMapBuffer();
        
        kinTex = new TextureManager();
        kinDepthTex = new TextureManager();
        
        // chess muss die proportionen des realen chesss haben
        // proportionen sind in der grafik schon richtig, deshalb keine
        // aspect anpassung
        glChessBoardWidth = 1.3f;
        chessBoard = new PropoImage((*scd->dataPath)+"textures/chessboard.jpg",
                                    scd->screenWidth, scd->screenHeight, glChessBoardWidth);
        glChessBoardHeight = chessBoard->getImgHeight();

        // 512 x 512 textur, nicht quadratische texture, skalieren nicht richtig...
        // proportion texture ist 1.8
        chessBoard2 = new PropoImage((*scd->dataPath)+"textures/phone_512.jpg",
                                     (int)((float)scd->screenWidth / 1.8f),
                                     scd->screenWidth, 2.f);

        // 512 x 512 textur, nicht quadratische texture, skalieren nicht richtig...
        // proportion texture ist 1.8
        chessBoard3 = new PropoImage((*scd->dataPath)+"textures/chessboard_512.jpg",
                                     (int)((float)scd->screenWidth / 1.8f),
                                     scd->screenWidth, realBoardWidth);
        
        cv::FileStorage fs((*scd->dataPath)+"calib_cam/xtion_color_640.yml", cv::FileStorage::READ);
        if ( fs.isOpened() )
        {
            fs["camera_matrix"] >> kpCalib.cameraMatrix;
            fs["distortion_coefficients"] >> kpCalib.distCoeffs;
            fs["image_width"] >> kpCalib.imgSize.width;
            fs["image_height"] >> kpCalib.imgSize.height;
        } else
        {
            printf("couldn´t open file\n");
        }
        
        chessRotMatr = glm::mat4(1.f);
        kalmanRot = new CvKalmanFilter(6, 3, 0.1f);
        kalmanRotZ = new CvKalmanFilter(6, 3, 0.05f);
        kalmanTrans = new CvKalmanFilter(6, 3, 0.1f);
        
//        kpCalib.beamerFovX = 2.f * std::atan2(0.5f, beamerThrowRatio);
//        kpCalib.beamerFovY = kpCalib.beamerFovX / kpCalib.beamerAspectRatio;
        // gemessen
        kpCalib.beamerFovX = 1.384f;
        kpCalib.beamerFovY = 1.11f;
        
        if(actDrawMode == CHECK_BEAMER_TO_OBJ || actDrawMode == CHECK_KIN_UNWARP)
        {
            cout << "load beamer to cam matrix" << endl;
            fs = cv::FileStorage(camBeamerMatrPath, cv::FileStorage::READ);
            if ( fs.isOpened() ) loadCamToBeamerCalib(fs, kpCalib);
        }
        
        if(actDrawMode == CHESS_TO_CAM || actDrawMode == CHECK_CAM_TO_BEAMER)
            kpCalib.invertMatr = false;
        
        colShader = shCol->getStdCol();
        shdr = shCol->getStdTex();
        blendShader = shCol->getStdTexAlpha();
//        blendShader = shCol->addCheckShader("texBlendShdr", "shaders/basic_tex.vert",
//                                                              "shaders/basic_tex_alpha.frag");
    }

    

    void SNTAugChessb2::initKinDependent()
    {
        kin->setImageRegistration(true);
        //kin->setMirror(false); // geht nicht bei depth stream, warum auch immer...
        // vermutlich problem mit verschiedenen auflösungen per stream
        
        kpCalib.imgSize.width = static_cast<float>(kin->getColorWidth());
        kpCalib.imgSize.height = static_cast<float>(kin->getColorHeight());
        
        kpCalib.depthImgSize.width = static_cast<float>(kin->getDepthWidth());
        kpCalib.depthImgSize.height = static_cast<float>(kin->getDepthHeight());

        kpCalib.camAspectRatio = static_cast<float>(kin->getColorWidth())
                                / static_cast<float>(kin->getColorHeight());
        kpCalib.camFovX = kpCalib.camAspectRatio * kpCalib.camFovY;
        // umrechnungsfaktor zwischen onistream FovY und dem gemessenen FovY
        cout << "kinFovX: " << kin->getDepthFovX() << endl;
        cout << "kinFovY: " << kin->getDepthFovY() << endl;
        
        kpCalib.camRealWorldXYScale = glm::vec2(kpCalib.camFovX / kin->getDepthFovX(),
                                                kpCalib.camFovY / kin->getDepthFovY());
        
        cv::initUndistortRectifyMap(kpCalib.cameraMatrix, kpCalib.distCoeffs, cv::Mat(),
                                    kpCalib.cameraMatrix,
                                    cv::Size(kin->getColorWidth(), kin->getColorHeight()),
                                    CV_16SC2, map1, map2);
        kinTex->allocate(kin->getColorWidth(), kin->getColorHeight(),
                         GL_RGBA8, GL_RGB, GL_TEXTURE_2D);
        kinDepthTex->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
                              GL_R16, GL_RED, GL_TEXTURE_2D);
        
        view = cv::Mat(kin->getColorHeight(), kin->getColorWidth(), CV_8UC3);
        viewDepth = cv::Mat(kin->getDepthHeight(), kin->getDepthWidth(), CV_16U);
        
        rview = cv::Mat(kin->getColorHeight(), kin->getColorWidth(), CV_8UC3);
        viewGray = cv::Mat(kin->getColorHeight(), kin->getColorWidth(), CV_8UC1);
        checkWarp = cv::Mat(kin->getColorHeight(), kin->getColorWidth(), CV_8UC3);
        checkWarpDepth = cv::Mat(kin->getDepthHeight(), kin->getDepthWidth(), CV_16UC1);
        
        isInited = true;
    }
    
    
    void SNTAugChessb2::update(double time, double dt)
    {
        cv::Mat uploadData, uploadDepth;
        
        if (isInited)
        {
        	bool gotNewColFrame = false;

            // depth Frame Operations
            if (actDrawMode == CHECK_KIN_OVERLAY)
            {
            	gotNewColFrame = kin->uploadColorImg();
                kin->uploadDepthImg(false);
            }
            
            // color Frame Operations
            if (gotNewColFrame)
            {
            	frameNr = kin->getColFrameNr();
                // take undistorted kin Color Image, since for this the Kinect internal
                // calibration works
                view.data = kin->getActColorImg();
//                view.data = testChessFile.data;
                
                uploadData = view;
                
                if ( frameNr > 20
                    && (actDrawMode == CHESS_TO_CAM
                        || (actDrawMode == CHECK_CAM_TO_BEAMER && gotCamToBeamer < getNrCheckSamples)
                        || (actDrawMode == SAVE_CAM_TO_BEAMER && gotCamToBeamer < getNrCheckSamples)))
                {
                    bool found = cv::findChessboardCorners(view, kpCalib.boardSize, pointbuf,
                                                           cv::CALIB_CB_ADAPTIVE_THRESH |
                                                           //cv::CALIB_CB_FAST_CHECK |
                                                           cv::CALIB_CB_NORMALIZE_IMAGE);
                    
                    if (found)
                    {
                        cout << "found" << endl;
                        chessRotMatr = getRotRealToGL(kpCalib);
                        
                        // get perspective transformation
                        kpCalib.camBeamerPerspTrans = getPerspTrans(pointbuf);
                        if (actDrawMode == SAVE_CAM_TO_BEAMER) getRealWorldKinBeamerOffs(kpCalib);
                        cv::drawChessboardCorners(view, kpCalib.boardSize, cv::Mat(pointbuf), found);
                        gotCamToBeamer++;
                        
                        // save data
                        if (gotCamToBeamer >= getNrCheckSamples && actDrawMode == SAVE_CAM_TO_BEAMER)
                            saveCamToBeamerCalib(camBeamerMatrPath, kpCalib);
                    }
                }
                
                if( actDrawMode == CHESS_TO_CAM
                   || actDrawMode == SAVE_CAM_TO_BEAMER
                   || (actDrawMode == CHECK_CAM_TO_BEAMER && gotCamToBeamer < getNrCheckSamples))
                {
                    cv::remap(view, rview, map1, map2, cv::INTER_LINEAR);
                    uploadData = rview;

                    glBindTexture(GL_TEXTURE_2D, kinTex->getId());
                    glTexSubImage2D(GL_TEXTURE_2D,             // target
                                    0,                          // First mipmap level
                                    0, 0,                       // x and y offset
                                    kin->getColorWidth(),
                                    kin->getColorHeight(),
                                    GL_RGB,
                                    GL_UNSIGNED_BYTE,
                                    uploadData.data);
                }

                if(actDrawMode == CHECK_KIN_UNWARP)
                {
//                    cv::remap(view, rview, map1, map2, cv::INTER_LINEAR);
//                    uploadData = rview;
                    
                    cv::warpPerspective(view, checkWarp, kpCalib.camBeamerPerspTrans, kpCalib.imgSize);
                    uploadData = checkWarp;
                    
                    glBindTexture(GL_TEXTURE_2D, kinTex->getId());
                    glTexSubImage2D(GL_TEXTURE_2D,             // target
                                    0,                          // First mipmap level
                                    0, 0,                       // x and y offset
                                    kin->getColorWidth(),
                                    kin->getColorHeight(),
                                    GL_RGB,
                                    GL_UNSIGNED_BYTE,
                                    uploadData.data);
                }
                
                // calibrate obj to beamer, result is a rotation and translation matrix
                // how is the object translated and rotated from the perspective of the camera?
                if (actDrawMode == CHECK_BEAMER_TO_OBJ && frameNr > 20)
                {
                    if (switchBeamerToObj == false && gotBeamerToObj < beamerToObjIntv)
                    {
                        //cv::remap(view, rview, map1, map2, cv::INTER_LINEAR);
                        cv::warpPerspective(view, checkWarp, kpCalib.camBeamerPerspTrans, kpCalib.imgSize);
                        //cv::imwrite("/Users/useruser/Desktop/unwrap.jpg", checkWarp);
                        
                        bool found = cv::findChessboardCorners(checkWarp, kpCalib.boardSize, pointbuf,
                                                               cv::CALIB_CB_ADAPTIVE_THRESH |
                                                              // cv::CALIB_CB_FAST_CHECK |
                                                               cv::CALIB_CB_NORMALIZE_IMAGE);
                        
                        if (found)
                        {
                            //cout << "found object" << endl;
                            kpCalib.invertMatr = false;
                            objRotMatr = getRotRealToCamToGL(kpCalib);
                            gotBeamerToObj++;
                        }
                    } else
                    {
                        if(switchBeamerToObj == false)
                        {
                            switchBeamerToObj = !switchBeamerToObj;
                            gotBeamerToObj = 0;
                        }
                    }
                }
            }
        }
    }
    
    
    void SNTAugChessb2::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        if (!isInited && kin->isReady())
        {
            initKinDependent();
        } else
        {
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            
            switch(actDrawMode)
            {
                case CHECK_KIN_OVERLAY:
                    drawKinOverlay();
                    break;
                case CHESS_TO_CAM :
                    drawChessToCam(time, dt);
                    break;
                case CHECK_CAM_TO_BEAMER:
                    drawCamToBeamer();
                    break;
                case SAVE_CAM_TO_BEAMER:
                    drawCamToBeamerInv();
                    break;
                case CHECK_KIN_UNWARP:
                    drawKinUnwrap();
                    break;
                case CHECK_BEAMER_TO_OBJ:
                    drawBeamerToObj();
                    break;
                default:
                    break;
            }
        }
    }

    
    void SNTAugChessb2::drawKinOverlay()
    {
        shdr->begin();
        shdr->setIdentMatrix4fv("m_pvm");
        shdr->setUniform1i("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, kin->getColorTexId());
        quad->draw();
        
        blendShader->begin();
        blendShader->setIdentMatrix4fv("m_pvm");
        blendShader->setUniform1f("alpha", 0.5f);
        blendShader->setUniform1i("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));
        quad->draw();
        
        // draw a cross centered to the screen for debugging
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        
        colShader->begin();
        colShader->setIdentMatrix4fv("m_pvm");
        yLine->draw(GL_LINES);
        xLine->draw(GL_LINES);
    }
    
    
    void SNTAugChessb2::drawChessToCam(double time, double dt)
    {
        float near = 0.1f, far = 100.f;
        glm::mat4 beamerMatr = glm::perspectiveFov(kpCalib.camFovY,
                                                   (float)scd->screenWidth,
                                                   (float)scd->screenHeight,
                                                   near, far);
        
        shdr->begin();
        shdr->setIdentMatrix4fv("m_pvm");
        shdr->setUniform1i("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, kinTex->getId());
        quad->draw();
        
        blendShader->begin();
        blendShader->setUniform1f("alpha", 0.f);
        blendShader->setUniform1i("tex", 0);
        
        beamerMatr = beamerMatr * chessRotMatr;
        blendShader->setUniformMatrix4fv("m_pvm", &beamerMatr[0][0]);

        glBindTexture(GL_TEXTURE_2D, chessBoard2->getTexId());
        chessBoard2->draw();
        
        // draw a cross centered to the screen for debugging
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        colShader->begin();
        colShader->setIdentMatrix4fv("m_pvm");
//        yLine->draw(GL_LINES);
//        xLine->draw(GL_LINES);
    }
    
    
    void SNTAugChessb2::drawCamToBeamer()
    {
        if(gotCamToBeamer < getNrCheckSamples)
        {
            shdr->begin();
            shdr->setIdentMatrix4fv("m_pvm");
            shdr->setUniform1i("tex", 0);
            glActiveTexture(GL_TEXTURE0);
            
            whiteQuad->draw();
            chessBoard->draw();
        } else
        {
            float near = 0.1f, far = 100.f;
            glm::mat4 beamerMatr = glm::perspectiveFov(kpCalib.camFovY,
                                                       (float)scd->screenWidth,
                                                       (float)scd->screenHeight,
                                                       near, far);
            
            glActiveTexture(GL_TEXTURE0);

            shdr->begin();
            shdr->setIdentMatrix4fv("m_pvm");
            shdr->setUniform1i("tex", 0);
            glBindTexture(GL_TEXTURE_2D, kinTex->getId());
            quad->draw();
            
            
            blendShader->begin();
            blendShader->setUniform1f("alpha", 0.5f);
            blendShader->setUniform1i("tex", 0);
            
            beamerMatr = beamerMatr * chessRotMatr;
            blendShader->setUniformMatrix4fv("m_pvm", &beamerMatr[0][0]);
            chessBoard2->draw();
        }
    }
    
    
    void SNTAugChessb2::drawCamToBeamerInv()
    {
        shdr->begin();
        shdr->setIdentMatrix4fv("m_pvm");
        shdr->setUniform1i("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        whiteQuad->draw();

        if (gotCamToBeamer < getNrCheckSamples)
        {
            chessBoard->draw();

        } else
        {
            // platziere und rotiere ein test-schachbrett, so dass die Kamera
            // ein exakt zentriertes und rotiertes Schachbrett mit der Breite 2.f
            // sieht
            float near = 0.1f, far = 100.f;
            glm::mat4 beamerMatr = glm::perspective( kpCalib.camFovY, kpCalib.camAspectRatio, near, far);
            float zDist = 1.f / std::tan(kpCalib.camFovX * 0.5f);
            
            // die zDist muss dann dem aktuellen abstand beamer Beamerektor entsprechen
            // deshalb hieraus skalierungsfaktor errechnen
            //float scaleFact = zDist / chessMid.z;
            float scaleFact = zDist / (kpCalib.beamerWallDist + kpCalib.camBeamerRealOffs.z);

            cout << "---" << endl;
            cout << glm::to_string(kpCalib.camBeamerRealOffs) << endl;
            cout << "chessRealWidth: " << kpCalib.chessRealWidth << endl;
            cout << "glChessBoardWidth: " << glChessBoardWidth << endl;
            cout << "beamerWallDist: " << kpCalib.beamerWallDist << endl;
            cout << "chessMid.z: " << kpCalib.chessRealMid.z << endl;
            cout << "scaleFact: " << scaleFact << endl;

            glm::mat4 look = glm::lookAt(kpCalib.chessNormal,
                                         glm::vec3(0.f, 0.f, 0.f),
                                         glm::vec3(0.f, 1.f, 0.f));

            // translation of the kamera to the beamer
            glm::mat4 trans = glm::translate(glm::mat4(1.f), kpCalib.chessRealMid * -scaleFact);
            
//            beamerMatr = beamerMatr * trans * kpCalib.chessRotXY * kpCalib.chessRotZ;
            beamerMatr = beamerMatr * trans * look;
        
            shdr->setUniformMatrix4fv("m_pvm", &beamerMatr[0][0]);
            glBindTexture(GL_TEXTURE_2D, chessBoard2->getTexId());
            chessBoard2->draw();
            
            
            // draw kinect view
            shdr->begin();
            shdr->setIdentMatrix4fv("m_pvm");
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, kinTex->getId());
            kinQuad->draw();
        }
    }
    
    
    void SNTAugChessb2::drawKinUnwrap()
    {
        shdr->begin();
        shdr->setIdentMatrix4fv("m_pvm");
        shdr->setUniform1i("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, kinTex->getId());
        quad->draw();
        
//        blendShader->begin();
//        blendShader->setIdentMatrix4fv("m_pvm");
//        blendShader->setUniform1f("alpha", 0.5f);
//        blendShader->setUniform1i("tex", 0);
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, kinDepthTex->getId());
//        quad->draw();
        
        // draw a cross centered to the screen for debugging
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        
        colShader->begin();
        colShader->setIdentMatrix4fv("m_pvm");
        yLine->draw(GL_LINES);
        xLine->draw(GL_LINES);
    }
    
    
    void SNTAugChessb2::drawBeamerToObj()
    {
        float near = 0.1f;
        float far = 400.f;
        
        glm::mat4 beamerMatr = glm::perspectiveFov(kpCalib.beamerFovY,
                                                   (float)scd->screenWidth,
                                                   (float)scd->screenHeight,
                                                   near, far);

        
//        if (switchBeamerToObj == 0 && gotBeamerToObj < beamerToObjIntv)
//        {
            shdr->begin();
            shdr->setUniform1i("tex", 0);
            shdr->setIdentMatrix4fv("m_pvm");
            whiteQuad->draw();

            // project test quad, size of the real test chessboard
            glm::vec3 tkinMid = glm::vec3(kpCalib.imgSize.width * 0.5f, kpCalib.imgSize.height * 0.5f, 0.f);
            getRealWorldCoord(tkinMid, kpCalib.camRealWorldXYScale);

            // berechne die Position der Kinect in Relation zum Beamer
            float rotXAlpha = M_PI - std::atan2(kpCalib.camBeamerNormal.y, kpCalib.camBeamerNormal.z);
            
            // y-Abstand der Kinect in Relation zum kinect Mittelpunkt
            float yOffs = std::sin(rotXAlpha) * tkinMid.z;
            float tKinectRectReal = yOffs / std::tan(rotXAlpha);
            tBeamerDistRectReal = ((tKinectRectReal - kpCalib.camBeamerRealOffs.z) + oldBeamerDistRectReal *10.f) / 11.f;
            oldBeamerDistRectReal = tBeamerDistRectReal;
            //cout << "tBeamerDistRectReal dist: " << tBeamerDistRectReal << endl;

//            chessOffsX = 0.1f;
//            chessOffsY = 0.2f;

            glm::mat4 scale = glm::translate(glm::mat4(1.f), glm::vec3((chessOffsX -0.4f) * 120.f,
                                                                       (0.6f - chessOffsY) * 100.f,
                                                                       -(tBeamerDistRectReal * 0.1f)));
            scale = beamerMatr * scale;
        
            shdr->setUniformMatrix4fv("m_pvm", &scale[0][0]);
            chessBoard3->draw();

        //} else
        
        if (!(switchBeamerToObj == 0 && gotBeamerToObj < beamerToObjIntv))
        {
            if ((gotBeamerToObj / 4) > beamerToObjIntv)
            {
                switchBeamerToObj = !switchBeamerToObj;
                gotBeamerToObj = 0;
            }
            
            blendShader->begin();
            blendShader->setUniform1i("tex", 0);
            blendShader->setUniform1f("alpha", 0.5f);
            
            objRotMatr = kpCalib.chessTrans;
//            objRotMatr = kpCalib.chessTrans * kpCalib.chessRotXY * kpCalib.chessRotZ;

            beamerMatr = beamerMatr * objRotMatr;
            blendShader->setUniformMatrix4fv("m_pvm", &beamerMatr[0][0]);
            glBindTexture(GL_TEXTURE_2D, chessBoard3->getTexId());
            chessBoard3->draw();
            
            gotBeamerToObj++;
        }
    }

    
    // in case of rectangular Beamer (ultra wide angle, zoom lens)
    // assume a centric Beamer cone anyway and caluculate a yOffs
    // depending on the distance and the characteristics of the beamer
    // gesucht ist eine Rotation/Transformation die den Beamerkonus der Kamera
    // auf den Beamerkonus des Beamers abbildet. also Kamera -> Beamer
    void SNTAugChessb2::getRealWorldKinBeamerOffs(kpCalibData& kp)
    {
        // berechne den abstand des Beamers zur wand
        // nimm dazu die breite des kompletten schachbretts und die throwRatio

        // berechne die breite des gesamten Beamer-Bildes
        // un berechne daraus den abstand des beamers zur wand
        kp.beamerWallDist = kp.beamerThrowRatio * kp.chessRealWidth * (2.f / glChessBoardWidth);
        cout << "kp.beamerWallDist: " << kp.beamerWallDist << endl;
        
        // berechne den abstand der Kinect zur Wand, nimm dazu den Mittelpunkt
        // des kinect color bildes
        glm::vec3 kinMid = glm::vec3(kp.imgSize.width * 0.5f, kp.imgSize.height * 0.5f, 0.f);
        getRealWorldCoord(kinMid, kp.camRealWorldXYScale);
        
        // get chess realworld coordinates
        std::vector<glm::vec3> realWorldCoord;
        for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
        {
            realWorldCoord.push_back( glm::vec3((*it).x, (*it).y, 0.f) );
            getRealWorldCoord(realWorldCoord.back(), kp.camRealWorldXYScale);
        }

        kp.chessRealMid = getChessMidPoint(realWorldCoord, kp);

        // get rotation matrix to rotate chess-PointBuf to Beamerector plane2
        glm::mat4 invRot = glm::mat4(RotationBetweenVectors(kp.chessNormal, glm::vec3(0.f, 0.f, -1.f)));

        for (std::vector<glm::vec3>::iterator it = realWorldCoord.begin(); it != realWorldCoord.end(); ++it)
            // take the realworld chess Coordinates, rotate them to fit the Beamerection plane of the kamera
            // rotate around the MidPoint of the chess
            (*it) = glm::vec3(invRot * glm::vec4((*it) - kp.chessRealMid, 1.f)) + kp.chessRealMid;

        
        // berechne die Position der Kinect in Relation zum Beamer
        float rotXAlpha = M_PI - std::atan2(kp.chessNormal.y, kp.chessNormal.z);
        
        // y-Abstand der Kinect in Relation zum kinect Mittelpunkt
        float yOffs = std::sin(rotXAlpha) * kinMid.z;
        kp.camWallRectReal = yOffs / std::tan(rotXAlpha);

        cout << "kp.camWallRectReal: " << kp.camWallRectReal << endl;

        
        // offset kinect-beamer bei ultraweit winkel Beamerektions-konus
        float realKinBeamerYOffs;

        float beamerWidth = kp.beamerWallDist / kp.beamerThrowRatio;
        float beamerHeight = ((float)scd->screenHeight / (float)scd->screenWidth) * beamerWidth;
        cout << "groesse Beamer w: " << beamerWidth << " h:" << beamerHeight << endl;
        
        // bei ULTRA_WIDE abstand des unteren Bildrandes zur Beamer Linse
        float vOffsBeamer = std::tan(kp.beamerLowEdgeAngle) * kp.beamerWallDist;
        cout << "vOffsBeamer: " << vOffsBeamer << endl;

        //cout << "abstand bildmitte: Beamerektor kante: " << BeamerectionHeight / 2 + vOffsBeamerektion << endl;
        
        // nur zur kontrolle, abstand y beamer/ kamera ohne die beamer model korrektur
        //cout << "abstand-y kin/Beamer real bei ultra-weit winkel: " << (beamerHeight / 2 + vOffsBeamerektion) - (chessMid.y - pk.y) << endl;
        
        //float pkY = (kp.chessRealMid.y + (std::sin(rotXAlpha) * kinMid.z));
        
        kp.camBeamerRealOffs = glm::vec3(kp.chessRealMid.x,
                                         (beamerHeight * 0.5f + vOffsBeamer) - (kp.chessRealMid.y - kinMid.y) - yOffs,
                                         kp.camWallRectReal - kp.beamerWallDist);
        
        cout << "kinect Offset: " << glm::to_string(kp.camBeamerRealOffs) << endl;

    }
    
    
    // convert the input coordinates of the kinect to realworld coordinates.
    // to do this, first correct the projection error of the camera
    // (this is align the camera and beamer plane)
    glm::mat4 SNTAugChessb2::getRotRealToCamToGL(kpCalibData& kp)
    {
        glm::mat4 out;
        std::vector<glm::vec3> realWorldCoord;
        glm::vec4 rotated;
        
        // get realworld coordinates, first with warped xy coordinates
        // which basically is incorrect, since the cv::warpPerspective already corrects it
        // but to get the correct z-values it´s handy
        float xzFactor = std::tan(kp.beamerFovX * 0.5f) * 2.f;  // stimmt noch nicht... wieso?
        float yzFactor = std::tan(kp.beamerFovY * 0.5f) * 2.f;  // stimmt!!!
        
        realWorldCoord.clear();
        for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
        {
            realWorldCoord.push_back( glm::vec3((*it).x, (*it).y, 0.f) );
            getRealWorldCoordUnwarp(realWorldCoord.back(), kp.camRealWorldXYScale,
                                    kp.camBeamerPerspTrans, true);
        }
        
        // recalculate the mid Point
        glm::vec3 chessMidPoint = getChessMidPoint(realWorldCoord, kp);

        
        // the z-Coordinate is incorrect, since perspectiveTransform has no effect on the
        // depth Value. To correct it rotate with normal of the cam/Beamer calibration
        // but only use the z-coordinate, since x,y should be already correct through
        // the unwraping
        for (std::vector<glm::vec3>::iterator it = realWorldCoord.begin(); it != realWorldCoord.end(); ++it)
        {
            rotated = glm::inverse(kp.camBeamerRotXY * kp.camBeamerRotZ) * glm::vec4((*it), 1.f);
            (*it).z = rotated.z - kp.camBeamerRealOffs.z;
           // cout << "rot: " << glm::to_string((*it)) << endl;
        }
        

        // now replace the y-, x- coordinates which the original ones from
        // the cv::warpPerspective
        int ind = 0;
        for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
        {
            glm::vec3 coordNorm = glm::vec3(((*it).x / (float)kin->getColorWidth() - 0.5f),
                                            (0.5f - (*it).y / (float)kin->getColorHeight()),
                                            0.f);
            
            coordNorm.x *= realWorldCoord[ind].z * xzFactor;
            coordNorm.y *= realWorldCoord[ind].z * yzFactor * 1.012f;
            
            realWorldCoord[ind].x = coordNorm.x;
            realWorldCoord[ind].y = coordNorm.y;

            ind++;
        }
        
        // recalculate the mid Point
        chessMidPoint = getChessMidPoint(realWorldCoord, kp);

        
        // get rotation around y and z axis
        kp.chessRotXY = calcChessRotXY(realWorldCoord, kp);
        // get the translation
        kp.chessTrans = calcChessTrans(realWorldCoord, kp, false);
        // get rotation around the z-axis
        kp.chessRotZ = calcChessRotZ(realWorldCoord, kp);
        
        
        // combine the three transformations
        if(!kp.invertMatr)
        {
            out = kp.chessTrans * kp.chessRotXY * kp.chessRotZ;
        } else {
            out = kp.chessTrans * kp.chessRotXY * kp.chessRotZ;
        }
        
       // cout << endl;

        return out;
    }
    
    
    glm::mat4 SNTAugChessb2::getRotRealToGL(kpCalibData& kp)
    {
        glm::mat4 out;
        glm::vec3 midPoint;        
        std::vector<glm::vec3> realWorldCoord;
        
        // get realworld coordinates
        realWorldCoord.clear();
        for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
        {
            realWorldCoord.push_back( glm::vec3((*it).x, (*it).y, 0.f) );
            getRealWorldCoord(realWorldCoord.back(), kp.camRealWorldXYScale);
        }
        
        // get rotation around y and z axis
        kp.chessRotXY = calcChessRotXY(realWorldCoord, kp);
        // get the translation
        kp.chessTrans = calcChessTrans(realWorldCoord, kp);
        // get rotation around the z-axis
        kp.chessRotZ = calcChessRotZ(realWorldCoord, kp);
        
        
        // combine the three transformations
        out = kp.chessTrans * kp.chessRotXY * kp.chessRotZ;
        
        return out;
    }
    
    
    void SNTAugChessb2::getRealWorldCoord(glm::vec3& inCoord, glm::vec2& _kinRealWorldXYScale)
    {
        // in case depth and color stream have different sizes
        inCoord.x = inCoord.x / static_cast<float>(kin->getColorWidth()) * static_cast<float>(kin->getDepthWidth());
        inCoord.y = inCoord.y / static_cast<float>(kin->getColorHeight()) * static_cast<float>(kin->getDepthHeight());
        
        const openni::DepthPixel* pDepthPix = (const openni::DepthPixel*)kin->getDepthFrame()->getData();
        int rowSize = kin->getDepthFrame()->getStrideInBytes() / sizeof(openni::DepthPixel);
        pDepthPix += rowSize * static_cast<int>(inCoord.y);
        pDepthPix += static_cast<int>(inCoord.x);
        
        openni::CoordinateConverter::convertDepthToWorld(*kin->getDepthStream(),
                                                         static_cast<int>(inCoord.x),
                                                         static_cast<int>(inCoord.y),
                                                         *pDepthPix,
                                                         &inCoord.x,
                                                         &inCoord.y,
                                                         &inCoord.z);

        inCoord *= glm::vec3(_kinRealWorldXYScale, 1.f);
    }
    
    
    // input are warped colorimage coordinates, so unwarp them to find the
    // correspoding depth values in the unwarped depth image
    void SNTAugChessb2::getRealWorldCoordUnwarp(glm::vec3& inCoord, glm::vec2& _kinRealWorldXYScale,
                                               cv::Mat& perspTrans, bool getWarpedXY)
    {
        // in case depth and color stream have different sizes
        std::vector<cv::Point2f> colorBufCoord, depthBufCoord;
        colorBufCoord.push_back(cv::Point2f(inCoord.x, inCoord.y));
        
        // unwarp this coordinates corresponding to the found perspective distortion of the kinect
        cv::perspectiveTransform(colorBufCoord, depthBufCoord, kpCalib.camBeamerInvPerspTrans);
        
        depthBufCoord[0].x = depthBufCoord[0].x / static_cast<float>(kin->getColorWidth())
                                * static_cast<float>(kin->getDepthWidth());
        depthBufCoord[0].y = depthBufCoord[0].y / static_cast<float>(kin->getColorHeight())
                                * static_cast<float>(kin->getDepthHeight());

        
        if(getWarpedXY)
        {
            const openni::DepthPixel* pDepthPix = (const openni::DepthPixel*)kin->getDepthFrame()->getData();
            int rowSize = kin->getDepthFrame()->getStrideInBytes() / sizeof(openni::DepthPixel);
            pDepthPix += rowSize * static_cast<int>(depthBufCoord[0].y);
            pDepthPix += static_cast<int>(depthBufCoord[0].x);
            
            openni::CoordinateConverter::convertDepthToWorld(*kin->getDepthStream(),
                                                             static_cast<int>(depthBufCoord[0].x),
                                                             static_cast<int>(depthBufCoord[0].y),
                                                             *pDepthPix,
                                                             &inCoord.x,
                                                             &inCoord.y,
                                                             &inCoord.z);
        } else
        {
            // take the coorected coordinates from the cv::warpperspective
            // with the corrected z-value
//            openni::DepthPixel* dp = new openni::DepthPixel[0];
//            dp[0] = static_cast<unsigned short>(inCoord.z);
//            cout << dp[0] << endl;
            const openni::DepthPixel* pDepthPix = (const openni::DepthPixel*)kin->getDepthFrame()->getData();
            int rowSize = kin->getDepthFrame()->getStrideInBytes() / sizeof(openni::DepthPixel);
            pDepthPix += rowSize * static_cast<int>(colorBufCoord[0].y);
            pDepthPix += static_cast<int>(colorBufCoord[0].x);
            
            openni::CoordinateConverter::convertDepthToWorld(*kin->getDepthStream(),
                                                             static_cast<int>(colorBufCoord[0].x),
                                                             static_cast<int>(colorBufCoord[0].y),
                                                             *pDepthPix,
                                                             &inCoord.x,
                                                             &inCoord.y,
                                                             &inCoord.z);
        }


        // the z-Coordinate is incorrect, since perspectiveTransform has no effect on the
        // depth Value
        inCoord *= glm::vec3(_kinRealWorldXYScale, 1.f);
    }

    
    glm::vec3 SNTAugChessb2::getChessMidPoint(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp)
    {
        glm::vec3 midPoint = glm::vec3(0.f);
        
        // calculate midpoints in the vertical axis
        // the outer most corners and iterate to the center
        vector<glm::vec3> hMidsEnd;
        
        for (short x=0;x<kp.boardSize.width;x++)
        {
            vector<glm::vec3> hMids;
            for (short h=0;h<kp.boardSize.height/2;h++)
            {
                glm::vec3 midP = _realWorldCoord[x + (h * kp.boardSize.width)]
                                + _realWorldCoord[x + (kp.boardSize.height -h -1) * kp.boardSize.width];
                hMids.push_back(midP * 0.5f);
            }
            
            // get mediums
            glm::vec3 medMidPH = glm::vec3(0.f, 0.f, 0.f);
            for (short h=0;h<kp.boardSize.height/2;h++)
                medMidPH += hMids[h];
            
            hMidsEnd.push_back(medMidPH / static_cast<float>(kp.boardSize.height/2));
        }
        
        vector<glm::vec3> vMids;
        int hMidsEndSizeHalf = static_cast<int>(hMidsEnd.size())/2;
        
        // iterate over all vMids and calc mediums for each pair
        for (int x=0;x<hMidsEndSizeHalf;x++)
        {
            glm::vec3 midP = hMidsEnd[x] + hMidsEnd[static_cast<int>(hMidsEnd.size()) -1 -x];
            vMids.push_back(midP * 0.5f);
        }
        
        midPoint = glm::vec3(0.f, 0.f, 0.f);
        for (short i=0;i<static_cast<short>(vMids.size());i++)
            midPoint += vMids[i];
        
        if(static_cast<int>(vMids.size()) > 0)
            midPoint /= static_cast<float>(vMids.size());
        
        return midPoint;
    }
    
    
    glm::mat4 SNTAugChessb2::calcChessRotXY(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp)
    {
        glm::mat4 outMat = glm::mat4(1.f);
        
        vector< glm::vec3 > normals;
        
        // calculate rotation
        // calculate normals for all corners
        for (short y=1;y<kp.boardSize.height;y++)
        {
            for (short x=0;x<kp.boardSize.width -1;x++)
            {
                glm::vec3 act = _realWorldCoord[y * kp.boardSize.width + x];
                glm::vec3 upper = _realWorldCoord[(y-1) * kp.boardSize.width + x];
                glm::vec3 right = _realWorldCoord[y * kp.boardSize.width + x+1];
                
                glm::vec3 vUp = glm::normalize(upper - act);
                glm::vec3 vRight = glm::normalize(right - act);
                
                normals.push_back( glm::normalize ( glm::cross(vUp, vRight) ) );
            }
        }
        
        // calculate medium normal
        glm::vec3 outNorm = glm::vec3(0.f, 0.f, 0.f);
        for (std::vector<glm::vec3>::iterator it = normals.begin(); it != normals.end(); ++it)
            if (!std::isnan((*it).x) && !std::isnan((*it).y) && !std::isnan((*it).z))
                outNorm += (*it);
        
        if ((int) normals.size() > 0)
        {
            outNorm /= (float)normals.size();
            
            // apply kalman filter
            kalmanRot->predict();
            kalmanRot->update(outNorm.x, outNorm.y, outNorm.z);
            cv::Mat smoothed = kalmanRot->getPrediction();
            
            kp.chessNormal.x = smoothed.at<float>(0);
            kp.chessNormal.y = smoothed.at<float>(1);
            kp.chessNormal.z = smoothed.at<float>(2);
            
            if (!kp.invertMatr)
                outMat = glm::mat4(RotationBetweenVectors(kp.chessNormal, glm::vec3(0.f, 0.f, -1.f)));
            else
                outMat = glm::mat4(RotationBetweenVectors(glm::vec3(0.f, 0.f, -1.f), kp.chessNormal));
        }
        
        return outMat;
    }
    
    
    glm::mat4 SNTAugChessb2::calcChessRotZ(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp)
    {
        glm::mat4 outMat = glm::mat4(1.f);
        glm::mat4 chessRotInvXYMatr;
        
        if (!kp.invertMatr)
            chessRotInvXYMatr = glm::mat4(RotationBetweenVectors(kp.chessNormal, glm::vec3(0.f, 0.f, -1.f)));
        else
            chessRotInvXYMatr = glm::mat4(RotationBetweenVectors(glm::vec3(0.f, 0.f, -1.f), kp.chessNormal));

        vector< glm::vec4 > leftVecs;
        
        // rotate the chess into the x-y plane
        chessbXyPlane.clear();
        for (std::vector<glm::vec3>::iterator it = _realWorldCoord.begin(); it != _realWorldCoord.end(); ++it)
            chessbXyPlane.push_back( chessRotInvXYMatr * glm::vec4((*it).x, (*it).y, (*it).z, 0.f) );
        
        // calculate rotation
        // calculate vector pointing to +x for all corners
        for (short y=0;y<kp.boardSize.height;y++)
        {
            for (short x=0;x<kp.boardSize.width -1;x++)
            {
                glm::vec4 act = chessbXyPlane[y * kp.boardSize.width + x];
                glm::vec4 right = chessbXyPlane[y * kp.boardSize.width + x+1];
                leftVecs.push_back( glm::normalize ( right - act ) );
            }
        }
        
        // calculate medium leftVec
        glm::vec4 outLeftVec = glm::vec4(0.f, 0.f, 0.f, 0.f);
        for (std::vector<glm::vec4>::iterator it = leftVecs.begin(); it != leftVecs.end(); ++it)
            if (!std::isnan((*it).x) && !std::isnan((*it).y) && !std::isnan((*it).z))
                outLeftVec += (*it);
        
        if (static_cast<int>(leftVecs.size()) > 0)
        {
            outLeftVec /= (float)leftVecs.size();
            
            // apply kalman filter
            kalmanRotZ->predict();
            kalmanRotZ->update(outLeftVec.x, outLeftVec.y, outLeftVec.z);
            cv::Mat smoothed = kalmanRotZ->getPrediction();
            
            glm::vec3 xyVec = glm::vec3(smoothed.at<float>(0), smoothed.at<float>(1), 0.f);
            
            if(!kp.invertMatr)
                outMat = glm::mat4(RotationBetweenVectors(glm::vec3(1.f, 0.f, 0.f), xyVec));
            else
                outMat = glm::mat4(RotationBetweenVectors(xyVec, glm::vec3(1.f, 0.f, 0.f)));
        }
        
        return outMat;
    }
    
    
    glm::mat4 SNTAugChessb2::calcChessTrans(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp, bool normalize)
    {
        glm::mat4 out;

        // calculate the size of the board in relation to boardSize,
        // use this to convert real to normalized opengl coordinates
        float widthReal = 0.f;
        
        for (short h=0;h<kp.boardSize.height;h++)
            widthReal += glm::length(_realWorldCoord[h * kp.boardSize.width]
                                     - _realWorldCoord[(h+1) * kp.boardSize.width -1]);
        
        widthReal /= static_cast<float>(kp.boardSize.height);
        widthReal *= (static_cast<float>(kp.boardSize.width +1) / static_cast<float>(kp.boardSize.width -1));
        kp.chessRealWidth = widthReal;
        
        // dieser faktor bezieht sich auf einen bestimmten abstand zum objekt
        realToNorm = 2.f / widthReal;
        
        kp.chessRealMid = getChessMidPoint(_realWorldCoord, kp);
        
        if(kp.chessRealMid.x != 0.f
           && kp.chessRealMid.y != 0.f
           && kp.chessRealMid.z != 0.f)
        {
            // scale medMidPV to normalized coordinates
            if(normalize)
                kp.chessRealMid *= realToNorm;
            else
                kp.chessRealMid *= coordNormFact;   // leave the coordinates as is, only scale by 0.1f
                                                    // for better handling
            
            // apply kalman filter
            if (std::isfinite(kp.chessRealMid.x)
                && std::isfinite(kp.chessRealMid.y)
                && std::isfinite(kp.chessRealMid.z))
            {
                kalmanTrans->predict();
                kalmanTrans->update(kp.chessRealMid.x,
                                    kp.chessRealMid.y,
                                    std::min(std::max(kp.chessRealMid.z, 0.0001f), 1000.f));
                
                cv::Mat smoothed = kalmanTrans->getPrediction();
                
                if(!kp.invertMatr)
                    out = glm::translate(glm::mat4(1.f), glm::vec3(smoothed.at<float>(0),
                                                                   smoothed.at<float>(1),
                                                                   smoothed.at<float>(2) * -1.f));
                else
                    out = glm::translate(glm::mat4(1.f), glm::vec3(-smoothed.at<float>(0),
                                                                   -smoothed.at<float>(1),
                                                                   -(smoothed.at<float>(2) * -1.f)));
            }
        }
        
        return out;
    }
    
    
    glm::mat4 SNTAugChessb2::rtMatToGlm(int imgW, int imgH, cv::Mat& rVec, cv::Mat& tVec, bool inv)
    {
        glm::mat4 outM = glm::mat4(1.f);
        glm::vec3 transV;
        
        if(inv)
        {
            transV = glm::vec3(static_cast<float>(tVec.at<double>(0) * -1.0),
                               static_cast<float>(tVec.at<double>(1)),
                               static_cast<float>(tVec.at<double>(2)));
            outM = glm::translate(outM, transV);
            
            outM = glm::rotate(outM, (float)rVec.at<double>(0) * -1.f, glm::vec3(1.f, 0.f, 0.f));
            outM = glm::rotate(outM, (float)rVec.at<double>(1), glm::vec3(0.f, 1.f, 0.f));
            outM = glm::rotate(outM, (float)rVec.at<double>(2), glm::vec3(0.f, 0.f, 1.f));
        } else
        {
            transV = glm::vec3(static_cast<float>(tVec.at<double>(0)),
                               static_cast<float>(tVec.at<double>(1) * -1.0),
                               static_cast<float>(tVec.at<double>(2) * -1.0));
            outM = glm::translate(outM, transV);
            
            outM = glm::rotate(outM, static_cast<float>(rVec.at<double>(0)), glm::vec3(1.f, 0.f, 0.f));
            outM = glm::rotate(outM, static_cast<float>(rVec.at<double>(1) * -1.0), glm::vec3(0.f, 1.f, 0.f));
            outM = glm::rotate(outM, static_cast<float>(rVec.at<double>(2) * -1.0), glm::vec3(0.f, 0.f, 1.f));
        }
        
        return outM;
    }
    
    
    glm::mat4 SNTAugChessb2::getBeamerFromCv(cv::Mat& camMatr, float calibWidth, float calibHeight, float znear,
                                           float zfar, bool yUp)
    {
        glm::mat4 outM = glm::mat4(1.f);
        
        // See http://strawlab.org/2011/11/05/augmented-reality-with-OpenGL/
        //if (!intrinsic_valid) {throw "invalid intrinsic";}
        
        float depth = zfar - znear;
        float q = -(zfar + znear) / depth;
        float qn = -2 * (zfar * znear) / depth;
        
        float x0=0;
        float y0=0;
        
        float width = calibWidth;
        float height = calibHeight;
        
        if (yUp)
        {
            outM[0][0] = 2.f * camMatr.at<double>(0) / width;
            outM[1][0] = -2.f * camMatr.at<double>(1) / width;
            outM[1][1] = -2.f * camMatr.at<double>(4) / height;
            outM[2][0] = (width - 2.f * camMatr.at<double>(2) + 2.f * x0) / width;
            outM[2][1] = (height - 2.f * camMatr.at<double>(5) + 2.f * y0) / height;
            outM[2][2] = q;
            outM[2][3] = -1.f;
            outM[3][2] = qn;
            
        } else
        {
            outM[0][0] = 2.f * camMatr.at<double>(0) / width;
            outM[1][0] = -2.f * camMatr.at<double>(1) / width;
            outM[1][1] = 2.f * camMatr.at<double>(4) / height;
            outM[2][0] = (width - 2.f * camMatr.at<double>(2) + 2.f * x0) / width;
            outM[2][1] = (-height - 2.f * camMatr.at<double>(5) + 2.f * y0) / height;
            outM[2][2] = q;
            outM[2][3] = -1.f;
            outM[3][2] = qn;
        }
        
        return outM;
    }
    
    
    cv::Mat SNTAugChessb2::getPerspTrans(std::vector<cv::Point2f>& pointbuf)
    {
        cv::Mat out;
        vector<cv::Point2f> inPoint;
        vector<cv::Point2f> dstPoint;
        
        // we don´t take the far out edges of the chessboard
        // but the next inner edges.
        float chessW = glChessBoardWidth * static_cast<float>(kpCalib.boardSize.width -1)
                        / static_cast<float>(kpCalib.boardSize.width +1)
                        * 0.5f * kpCalib.imgSize.width;
        float chessH = glChessBoardHeight * static_cast<float>(kpCalib.boardSize.height-1)
                        / static_cast<float>(kpCalib.boardSize.height +1)
                        * 0.5f * kpCalib.imgSize.height;
        
        float chessOffsX = (2.f - glChessBoardWidth) * 0.25f * kpCalib.imgSize.width
                            + chessW / static_cast<float>(kpCalib.boardSize.width -1);
        float chessOffsY = (2.f - glChessBoardHeight) * 0.25f * kin->getColorHeight()
                            + chessH / static_cast<float>(kpCalib.boardSize.height -1);
        
        // lower left (in point upper, weil kommt auf d. kopf rein)
        for( int y = 0; y < 2; y++ )
            for( int x = 0; x < 2; x++ )
                dstPoint.push_back(cv::Point2f(float(x) * chessW + chessOffsX,
                                               float(y) * chessH + chessOffsY));
        
        inPoint.push_back(pointbuf[0]);
        inPoint.push_back(pointbuf[kpCalib.boardSize.width-1]);
        inPoint.push_back(pointbuf[(kpCalib.boardSize.height-1) * kpCalib.boardSize.width]);
        inPoint.push_back(pointbuf[(kpCalib.boardSize.height-1) * kpCalib.boardSize.width + kpCalib.boardSize.width -1]);

        out = cv::getPerspectiveTransform(inPoint, dstPoint);
        return out;
    }


    void SNTAugChessb2::loadCamToBeamerCalib(cv::FileStorage& fs, kpCalibData& kp)
    {
        
        fs["camera_matrix"] >> kp.cameraMatrix;
        fs["distortion_coefficients"] >> kp.distCoeffs;
        
        fs["invertMatr"] >> kp.invertMatr;
        fs["image_width"] >> kp.imgSize.width;
        fs["image_height"] >> kp.imgSize.height;
        fs["depth_image_width"] >> kp.depthImgSize.width;
        fs["depth_image_height"] >> kp.depthImgSize.height;

        fs["board_width"] >> kp.boardSize.width;
        fs["board_height"] >> kp.boardSize.height;
        fs["nrSamples"] >> kp.nrSamples;
        fs["beamerAspectRatio"] >> kp.beamerAspectRatio;
        fs["beamerFovX"] >> kp.beamerFovX;
        fs["beamerFovY"] >> kp.beamerFovY;
        fs["beamerLowEdgeAngle"] >> kp.beamerLowEdgeAngle;
        fs["beamerThrowRatio"] >> kp.beamerThrowRatio;
        fs["beamerWallDist"] >> kp.beamerWallDist;
        fs["camAspectRatio"] >> kp.camAspectRatio;
        fs["camWallRectReal"] >> kp.camWallRectReal;
        
        fs["chessRealWidth"] >> kp.chessRealWidth;
        fs["camFovX"] >> kp.camFovX;
        fs["camFovY"] >> kp.camFovY;
        
        cv::Mat pRotXY, pRotZ, pTrans;
        fs["camBeamerRotXY"] >> pRotXY;
        fs["camBeamerRotZ"] >> pRotZ;
        fs["camBeamerTrans"] >> pTrans;
        fs["camBeamerPerspTrans"] >> kp.camBeamerPerspTrans;
        fs["camBeamerInvPerspTrans"] >> kp.camBeamerInvPerspTrans;

        kp.camBeamerRotXY = cvMatToGlm(pRotXY);
        kp.camBeamerRotZ = cvMatToGlm(pRotZ);
        kp.camBeamerTrans = cvMatToGlm(pTrans);
        
        if(!kp.invertMatr)
        {
            kp.camBeamerRotXY = glm::inverse(kp.camBeamerRotXY);
            kp.camBeamerRotZ = glm::inverse(kp.camBeamerRotZ);
        }
        
        cv::Mat beamerRealMid, camBeamerNormal, camBeamerRealOffs;
        cv::Mat camRealWorldXYScale;
        fs["camBeamerRealOffs"] >> camBeamerRealOffs;
        fs["beamerRealMid"] >> beamerRealMid;
        fs["camBeamerNormal"] >> camBeamerNormal;
        fs["camRealWorldXYScale"] >> camRealWorldXYScale;
        
        kp.camBeamerRealOffs = glm::vec3(camBeamerRealOffs.at<float>(0),
                                         camBeamerRealOffs.at<float>(1),
                                         camBeamerRealOffs.at<float>(2));

        kp.beamerRealMid = glm::vec3(beamerRealMid.at<float>(0),
                                     beamerRealMid.at<float>(1),
                                     beamerRealMid.at<float>(2));
        
        kp.camBeamerNormal = glm::vec3(camBeamerNormal.at<float>(0),
                                       camBeamerNormal.at<float>(1),
                                       camBeamerNormal.at<float>(2));
        
        kp.camRealWorldXYScale = glm::vec2(camRealWorldXYScale.at<float>(0),
                                           camRealWorldXYScale.at<float>(1));

        std::string beamMod;
        fs["beamerModel"] >> beamMod;
        
        if (std::strcmp(beamMod.c_str(), "ZOOM") == 0)
            kp.beamModel = ZOOM;
        
        if (std::strcmp(beamMod.c_str(), "WIDE") == 0)
            kp.beamModel = WIDE;
        
        if (std::strcmp(beamMod.c_str(), "ULTRA_WIDE") == 0)
            kp.beamModel = ULTRA_WIDE;
        
        camBeamerRotMatr = kp.camBeamerTrans * kp.camBeamerRotXY * kp.camBeamerRotZ;
        
        gotCamToBeamer = getNrCheckSamples;
    }
    
    
    void SNTAugChessb2::saveCamToBeamerCalib(std::string _filename, kpCalibData& kp)
    {
        printf("saving camera to beamer calib \n");
        
        cv::FileStorage fs( _filename, cv::FileStorage::WRITE );
        
        fs << "camera_matrix" << kp.cameraMatrix;
        fs << "distortion_coefficients" << kp.distCoeffs;
        
        fs << "invertMatr" << kp.invertMatr;
        fs << "image_width" << kp.imgSize.width;
        fs << "image_height" << kp.imgSize.height;
        fs << "depth_image_width" << kp.depthImgSize.width;
        fs << "depth_image_height" << kp.depthImgSize.height;
        fs << "board_width" << kp.boardSize.width;
        fs << "board_height" << kp.boardSize.height;
        fs << "nrSamples" << kp.nrSamples;
        fs << "beamerAspectRatio" << kp.beamerAspectRatio;
        fs << "beamerFovX" << kp.beamerFovX;
        fs << "beamerFovY" << kp.beamerFovY;

        fs << "beamerLowEdgeAngle" << kp.beamerLowEdgeAngle;
        fs << "beamerThrowRatio" << kp.beamerThrowRatio;
        fs << "beamerWallDist" << kp.beamerWallDist;
        fs << "camAspectRatio" << kp.camAspectRatio;
        fs << "camWallRectReal" << kp.camWallRectReal;
        fs << "chessRealWidth" << kp.chessRealWidth;
        fs << "camFovX" << kp.camFovX;
        fs << "camFovY" << kp.camFovY;
        
        fs << "camBeamerRotXY" << glmToCvMat(kp.chessRotXY);
        fs << "camBeamerRotZ" << glmToCvMat(kp.chessRotZ);
        glm::mat4 camTrans = glm::translate(glm::mat4(1.f), kp.camBeamerRealOffs);
        fs << "camBeamerTrans" << glmToCvMat(camTrans);
        fs << "camBeamerPerspTrans" << kp.camBeamerPerspTrans;
        fs << "camBeamerInvPerspTrans" << kp.camBeamerPerspTrans.inv();


        cv::Mat glmVec = cv::Mat(3, 1, CV_32F);
        glmVec.at<float>(0) = kp.chessRealMid.x;
        glmVec.at<float>(1) = kp.chessRealMid.y;
        glmVec.at<float>(2) = kp.chessRealMid.z;
        fs << "beamerRealMid" << glmVec;
        
        glmVec.at<float>(0) = kp.chessNormal.x;
        glmVec.at<float>(1) = kp.chessNormal.y;
        glmVec.at<float>(2) = kp.chessNormal.z;
        fs << "camBeamerNormal" << glmVec;
        
        glmVec.at<float>(0) = kp.camBeamerRealOffs.x;
        glmVec.at<float>(1) = kp.camBeamerRealOffs.y;
        glmVec.at<float>(2) = kp.camBeamerRealOffs.z;
        fs << "camBeamerRealOffs" << glmVec;

        cv::Mat glmVec2 = cv::Mat(2, 1, CV_32F);
        glmVec2.at<float>(0) = kp.camRealWorldXYScale.x;
        glmVec2.at<float>(1) = kp.camRealWorldXYScale.y;
        fs << "camRealWorldXYScale" << glmVec2;
        
        switch(kp.beamModel)
        {
            case ZOOM: fs << "beamerModel" << "ZOOM";
                break;
            case WIDE: fs << "beamerModel" << "WIDE";
                break;
            case ULTRA_WIDE: fs << "beamerModel" << "ULTRA_WIDE";
                break;
        }
    }
    
    
    cv::Mat SNTAugChessb2::glmToCvMat(glm::mat4& mat)
    {
        cv::Mat out = cv::Mat(4, 4, CV_32F);
        for (short j=0;j<4;j++)
            for (short i=0;i<4;i++)
                out.at<float>(j*4 + i) = mat[i][j];

        return out;
    }
    

    glm::mat4 SNTAugChessb2::cvMatToGlm(cv::Mat& _mat)
    {
        glm::mat4 out = glm::mat4(1.f);
        for (short j=0;j<4;j++)
            for (short i=0;i<4;i++)
                out[i][j] = _mat.at<float>(j*4 + i);
        
        return out;
    }
    
    
    glm::mat4 SNTAugChessb2::cvMat33ToGlm(cv::Mat& _mat)
    {
        glm::mat4 out = glm::mat4(1.f);
        for (short j=0;j<3;j++)
            for (short i=0;i<3;i++)
                out[i][j] = _mat.at<double>(j*3 + i);
                
                return out;
    }
    
    
    void SNTAugChessb2::onKey(int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS)
        {
            //            switch (key)
            //            {
            //            }
        }
    }
    
    
    void SNTAugChessb2::onCursor(GLFWwindow* window, double xpos, double ypos)
    {
        chessOffsX = static_cast<float>(xpos / static_cast<double>(scd->screenWidth));
        chessOffsY = static_cast<float>(ypos / static_cast<double>(scd->screenHeight));
    }
    
    
    void SNTAugChessb2::onMouseButton(GLFWwindow* window, int button, int action, int mods)
    {
    }
    
    
    SNTAugChessb2::~SNTAugChessb2()
    {
        delete colShader;
        delete blendShader;
        delete quad;
        delete noFlipQuad;
        delete kinQuad;
        delete whiteQuad;
        delete xLine;
        delete yLine;
        delete kinTex;
        delete chessBoard;
        delete chessBoard2;
        delete kalmanRot;
        delete kalmanRotZ;
        delete kalmanTrans;
    }
    
}
