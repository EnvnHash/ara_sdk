//
// SNTMapping.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  mirroring has to be off!

#include "SNTMapping.h"

using namespace std;
using namespace cv;

namespace tav
{
    SNTMapping::SNTMapping(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs), useFishEye(false), beamerThrowRatio(0.48684), calibCamToBeamer(false),
    nrBeamerToCamSnapshots(5),
    beamerUpsideDown(false), gotPose(false), gotCamBeamerMapping(false)
    {
    	kin = static_cast<KinectInput*>(scd->kin);
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        quad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
                        glm::vec3(0.f, 0.f, 1.f),
                        0.f, 0.f, 0.f, 1.f,
                        nullptr, 1, true);
        
        noFlipQuad = scd->stdQuad;

        kinQuad = new Quad(-1.0f, .75f, 0.25f, 0.25f,
                           glm::vec3(0.f, 0.f, 1.f),
                           0.f, 0.f, 0.f, 1.f,
                           nullptr, 1, true);
        
        kinDepthQuad = new Quad(-0.75f, .75f, 0.25f, 0.25f,
                                glm::vec3(0.f, 0.f, 1.f),
                                0.f, 0.f, 0.f, 1.f,
                                nullptr, 1, true);
        
        whiteQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
                             glm::vec3(0.f, 0.f, 1.f),
                             1.f, 1.f, 1.f, 1.f);
        
        shdr = shCol->getStdTex();

        kinTex = new TextureManager();
        kinDepthTex = new TextureManager();
        
        chessBoardWidth = 1.5f;
        chessBoard = new PropoImage((*scd->dataPath)+"textures/chessBoard.jpg",
                                    _scd->screenWidth, _scd->screenHeight, chessBoardWidth);
        chessBoardHeight = chessBoard->getImgHeight();
        boardSize = cv::Size(8,4);
        
        cv::FileStorage fs((*scd->dataPath)+"calib_cam/xtion_color.yml", cv::FileStorage::READ);
        if ( fs.isOpened() )
        {
            fs["camera_matrix"] >> cameraMatrix;
            fs["distortion_coefficients"] >> distCoeffs;
            
            fovX = 58.f / 360.f * 2.f * M_PI;
            fovY = 45.f / 360.f * 2.f * M_PI;
            fovD = 70.f / 360.f * 2.f * M_PI;

        } else {
            printf("couldn´t open file\n");
        }

//        cv::FileStorage dfs((*scd->dataPath)+"calib_cam/xtion_fisheye_depth.yml", cv::FileStorage::READ);
//        if ( dfs.isOpened() )
//        {
//            dfs["camera_matrix"] >> dCameraMatrix;
//            dfs["distortion_coefficients"] >> dDistCoeffs;
//        } else {
//            printf("couldn´t open file\n");
//        }
//        
        camBeamerMatrPath = (*scd->dataPath)+"calib_cam/xtion_camToBeamer.yml";

        if(!calibCamToBeamer)
        {
            cv::FileStorage pfs(camBeamerMatrPath, cv::FileStorage::READ);
            if ( pfs.isOpened() )
            {
                camBeamerMat = Mat::eye(3, 3, CV_64F);
                pfs["perspective_matrix"] >> camBeamerMat;
                pfs["distortion_coefficients"] >> distCoeffs;
                
                cout << "camBeamerMat: " << camBeamerMat << endl;
                
                pfs["perp_trans"] >> camBeamerPerspTrans;
                cout << "perp_trans: " << camBeamerPerspTrans << endl;

                camBeamerRvecs.push_back(cv::Mat());
                pfs["rvec"] >> camBeamerRvecs[0];
                std::cout << camBeamerRvecs[0] << endl;
                
                camBeamerTvecs.push_back(cv::Mat());
                pfs["tvec"] >> camBeamerTvecs[0];
                std::cout << camBeamerTvecs[0] << endl;
                
                int imgWidth, imgHeight;
                pfs["image_width"] >> imgWidth;
                pfs["image_height"] >> imgHeight;
                
                // normalize translation matrix
                camBeamerTransMatr = rtMatToGlm(imgWidth, imgHeight, camBeamerRvecs[0], camBeamerTvecs[0]);
            }

            gotCamBeamerMapping = true;
        }
        
        blendShader = shCol->addCheckShader("texBlendShdr", "shaders/basic_tex.vert",
                                                              "shaders/basic_tex_alpha.frag");
        
        // schmutziger offset, weil depth on color stream registration nicht richtig
        // funktioniert, wenn color 1280x960 und depth 640x480
        depthToColorOffset = glm::vec3(0.f, 0.08f, 0.f);
        
        mapCam = new GLMCamera(GLMCamera::FRUSTUM,
                               scd->screenWidth, scd->screenHeight,
                               -1.f, 1.f, -1.f, 1.f,
                               0.f, 0.f, 1.f,
                               0.f, 0.f, 0.f,
                               0.f, 1.f, 0.f,
                               1.f, 100.f,
                               fovD);
    }
    
    
    
    SNTMapping::~SNTMapping()
    {}
    
    
    
    void SNTMapping::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        //sendStdShaderInit(_shader);
        
        if (!isInited && kin->isReady())
        {
            kin->setCloseRange(false);
            kin->setImageRegistration(true);
            //kin->setMirror(false); // geht nicht bei depth stream, warum auch immer...
                                            // vermutlich problem mit verschiedenen auflösungen per stream
            
            imageSize = cv::Size(kin->getColorWidth(), kin->getColorHeight());
            cv::initUndistortRectifyMap(cameraMatrix, distCoeffs, cv::Mat(),
                                        cameraMatrix, imageSize, CV_16SC2, map1, map2);
            kinTex->allocate(kin->getColorWidth(), kin->getColorHeight(),
                             GL_RGBA8, GL_RGB, GL_TEXTURE_2D);
            view = cv::Mat(kin->getColorHeight(), kin->getColorWidth(), CV_8UC3);
            rview = cv::Mat(kin->getColorHeight(), kin->getColorWidth(), CV_8UC3);
            viewGray = cv::Mat(kin->getColorHeight(), kin->getColorWidth(), CV_8UC1);
            checkWarp = cv::Mat(kin->getColorHeight(), kin->getColorWidth(), CV_8UC3);

            //------------------------------------------------------
            
            /*
            imageSize = cv::Size(kin->getDepthWidth(), kin->getDepthHeight());
            cv::initUndistortRectifyMap(dCameraMatrix, dDistCoeffs, cv::Mat(),
                                        dCameraMatrix, imageSize, CV_16SC2, dMap1, dMap2);
            kinDepthTex->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
                                  GL_R8, GL_RED, GL_TEXTURE_2D);
            dView = cv::Mat(kin->getDepthHeight(), kin->getDepthWidth(), CV_8UC1);
            dRview = cv::Mat(kin->getDepthHeight(), kin->getDepthWidth(), CV_8UC1);

*/
            isInited = true;
            
        } else
        {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            //glBlendFunc(GL_ONE, GL_ZERO);

            updateKinectTex();
            
            if(!gotCamBeamerMapping)
            {
                // draw chessboard image
                shdr->begin();
                shdr->setIdentMatrix4fv("m_pvm");
                shdr->setUniform1i("tex", 0);
                whiteQuad->draw();
                chessBoard->draw();
                
                getCamBeamerMapping(time);

            } else if(!rview.empty() && !view.empty())
            {
                /*
                // try to find chessboard for pose estimation
                if (!gotPose)
                {
                    bool found = cv::findChessboardCorners(checkWarp, boardSize, pointbuf, cv::CALIB_CB_ADAPTIVE_THRESH |
                                                           cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE);

                    if (found)
                    {
                        cv::cvtColor(checkWarp, viewGray, CV_BGR2GRAY);

                        cv::cornerSubPix(viewGray, pointbuf,
                                         cv::Size(11,11), cv::Size(-1,-1),
                                         cv::TermCriteria( cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1 ));
                        
                        cv::drawChessboardCorners( checkWarp, boardSize, cv::Mat(pointbuf), found );
                        
                        std::vector< cv::Point3f > objectPoints;
                        
                        for( int i = 0; i < boardSize.height; i++ )
                            for( int j = 0; j < boardSize.width; j++ )
                                objectPoints.push_back(cv::Point3f(float(j) / float(boardSize.width),
                                                                   float(i) / float(boardSize.height),
                                                                   0));
                        
                        cv::solvePnPRansac(objectPoints,
                                           pointbuf,
                                           cameraMatrix, distCoeffs,
                                           rvec, tvec,
                                           false,                   // useExtrinsicGuess
                                           100,                     // iterationsCount
                                           8.0,                     // reprojectionError
                                           0.99,                    // confidence
                                           cv::noArray(),           // inliers
                                           cv::SOLVEPNP_ITERATIVE); // flags
                        
                        cout << rvec << endl;
                        cout << tvec << endl;
                        
                        gotPose = true;
                    }
                }
                 
                 */

                /*
                cv::warpPerspective(rview, checkWarp, camBeamerPerspTrans,
                                    cv::Size(kin->getColorWidth(), kin->getColorHeight()));

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, kinTex->getId());
                glTexSubImage2D(GL_TEXTURE_2D,             // target
                                0,                          // First mipmap level
                                0, 0,                       // x and y offset
                                kin->getColorWidth(),
                                kin->getColorHeight(),
                                GL_RGB,
                                GL_UNSIGNED_BYTE,
                                checkWarp.data);
                
                shdr->begin();
                shdr->setIdentMatrix4fv("m_pvm");
                shdr->setUniform1i("tex", 0);
                glActiveTexture(GL_TEXTURE0);
                quad->draw();
*/
                /*
                GLMCamera* cam = new GLMCamera(GLMCamera::PERSPECTIVE,
                                               scd->screenWidth, scd->screenHeight,
                                               -1.f, 1.f, -1.f, 1.f,
                                               0.f, 0.f, 1.f,
                                               0.f, 0.f, 0.f,
                                               0.f, 1.f, 0.f,
                                               1.f, 100.f,
                                               osc->blurOffs * 180.f);
                
                cam->setModelMatr(camBeamerTransMatr);
                cam->sendMVP(shdr->getProgram(), "m_pvm");

                glBindTexture(GL_TEXTURE_2D, chessBoard->getTexId());
                 quad->draw();
                 */
                
//                shdr->setIdentMatrix4fv("m_pvm");
//                glBindTexture(GL_TEXTURE_2D, kinTex->getId());
//                kinQuad->draw();
            }

            shdr->begin();
            shdr->setIdentMatrix4fv("m_pvm");
            shdr->setUniform1i("tex", 0);
            glActiveTexture(GL_TEXTURE0);
            
            glBindTexture(GL_TEXTURE_2D, kinTex->getId());
            quad->draw();

            /*
            glm::mat4 rotMatr = glm::mat4(1.f);
            // apply translation
            rotMatr = glm::translate(rotMatr, glm::vec3(tvec.at<double>(0),
                                                        tvec.at<double>(1) * -1.f,
                                                        tvec.at<double>(2) * -1.f));
            rotMatr = glm::rotate(rotMatr, (float)rvec.at<double>(0) * -1.f, glm::vec3(1.f, 0.f, 0.f));
            rotMatr = glm::rotate(rotMatr, (float)rvec.at<double>(1) * -1.f, glm::vec3(0.f, 1.f, 0.f));
            rotMatr = glm::rotate(rotMatr, (float)rvec.at<double>(2) * -1.f, glm::vec3(0.f, 0.f, 1.f));
            
            mapCam->setModelMatr(rotMatr);
            mapCam->sendMVP(shdr->getProgram(), "m_pvm");
            
//            glBindTexture(GL_TEXTURE_2D, chessBoard->getTexId());
//            quad->draw();
             */
            
            //        glActiveTexture(GL_TEXTURE0);
            //       glBindTexture(GL_TEXTURE_2D, kinTex->getId());
            //        quad->draw();
            
        }
    }

    

    void SNTMapping::updateKinectTex()
    {
        cv::Mat uploadData, uploadDepth;
        
        if (frameNr != kin->getColFrameNr())
        {
            frameNr = kin->getColFrameNr();

            view.data = kin->getActColorImg();

            cv::remap(view, rview, map1, map2, cv::INTER_LINEAR);
            uploadData = rview;

            if(beamerUpsideDown)
                flip(uploadDepth, uploadDepth, 0);
            
            glActiveTexture(GL_TEXTURE0);
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
        
        //---
        
        /*
        if (depthFrameNr != kin->getDepthFrameNr(useHisto))
        {
            dView.data = kin->getActDepthImg8(useHisto);
            
            if(useFishEye)
            {
                cv::remap(dView, dRview, dMap1, dMap2, cv::INTER_LINEAR);
                uploadDepth = dRview;
            } else {
                uploadDepth = dView;
            }
            
            if(beamerUpsideDown)
                flip(uploadDepth, uploadDepth, 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, kinDepthTex->getId(useHisto));
            glTexSubImage2D(GL_TEXTURE_2D,             // target
                            0,                          // First mipmap level
                            0, 0,                       // x and y offset
                            kin->getDepthWidth(),
                            kin->getDepthHeight(),
                            GL_RED,
                            GL_UNSIGNED_BYTE,
                            uploadDepth.data);
            
            depthFrameNr = kin->getDepthFrameNr(useHisto);
        }
         */
    }

    
    
    void SNTMapping::getCamBeamerMapping(double time)
    {
        // get chessboard corners
        if (!view.empty() && !rview.empty() && frameNr > 20 && (frameNr - oldFrameSnapNr) > 5 )
        {
            pointbuf.clear();
            bool found = false;
            
            if(!gotCamBeamerMapping)
            {
                if(snapshotCtr < nrBeamerToCamSnapshots)
                {
                    found = cv::findChessboardCorners(rview, boardSize, pointbuf,
                                                      cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK |
                                                      cv::CALIB_CB_NORMALIZE_IMAGE);
                    if (found)
                    {
                        cv::cvtColor(rview, viewGray, CV_BGR2GRAY);
                        cv::cornerSubPix(viewGray, pointbuf,
                                         cv::Size(11,11), cv::Size(-1,-1),
                                         cv::TermCriteria( cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1 ));

                        oldFrameSnapNr = frameNr;
                        
                        // get perspective transform
                        vector<cv::Point2f> inPoint;
                        vector<cv::Point3f> dstPoint;
                        
                        // we don´t take the far out edges of the chessboard
                        // but the next inner edges.
                        float chessW = chessBoardWidth * static_cast<float>(boardSize.width -1)
                                        / static_cast<float>(boardSize.width +1)
                                        * 0.5f * kin->getColorWidth();
                        float chessH = chessBoardHeight * static_cast<float>(boardSize.height-1)
                                        / static_cast<float>(boardSize.height +1)
                                        * 0.5f * kin->getColorHeight();
                        
                        float chessOffsX = (2.f - chessBoardWidth) * 0.25f * kin->getColorWidth()
                                            + chessW / static_cast<float>(boardSize.width -1);
                        float chessOffsY = (2.f - chessBoardHeight) * 0.25f * kin->getColorHeight()
                                            + chessH / static_cast<float>(boardSize.height -1);
                        
                        // lower left (in point upper, weil kommt auf d. kopf rein)
                        for( int i = 0; i < boardSize.height; i++ )
                            for( int j = 0; j < boardSize.width; j++ )
                                dstPoint.push_back(Point3f(float(j) / float(boardSize.width -1) * chessW + chessOffsX,
                                                           float(i) / float(boardSize.height -1) * chessH + chessOffsY,
                                                           0));
                        
                        objectPoints.push_back(dstPoint);
                        imagePoints.push_back(pointbuf);
                        
                        snapshotCtr++;
                        std::cout << "took Nr snapshots: " << snapshotCtr << endl;
                        
                        shdr->begin();
                        shdr->setIdentMatrix4fv("m_pvm");
                        shdr->setUniform1i("tex", 0);
                        quad->draw();
                    }
                } else
                {
                    // calculate beamer to Camera Matrix and DistCoeff
                    camBeamerMat = Mat::eye(3, 3, CV_64F);
                    camBeamerDistCoeffs = Mat::zeros(8, 1, CV_64F);

                    double rms = calibrateCamera(objectPoints, imagePoints,
                                                 cv::Size(kin->getColorWidth(), kin->getColorHeight()),
                                                 camBeamerMat,
                                                 camBeamerDistCoeffs,
                                                 camBeamerRvecs, camBeamerTvecs,
                                                 CALIB_FIX_K4|CALIB_FIX_K5);
                    ///*|CALIB_FIX_K3*/|CALIB_FIX_K4|CALIB_FIX_K5);
                    vector<cv::Point2f> inPoint;
                    vector<cv::Point2f> dstPoint;
                    
                    inPoint.push_back(imagePoints[0][0]);
                    dstPoint.push_back(cv::Point2f(objectPoints[0][0].x, objectPoints[0][0].y));
                    
                    inPoint.push_back(imagePoints[0][boardSize.width-1]);
                    dstPoint.push_back(cv::Point2f(objectPoints[0][boardSize.width-1].x,
                                                   objectPoints[0][boardSize.width-1].y));

                    inPoint.push_back(imagePoints[0][(boardSize.height-1) * boardSize.width + boardSize.width-1]);
                    dstPoint.push_back(cv::Point2f(objectPoints[0][(boardSize.height -1) * boardSize.width + boardSize.width-1].x,
                                                   objectPoints[0][(boardSize.height -1) * boardSize.width + boardSize.width-1].y));

                    inPoint.push_back(imagePoints[0][(boardSize.height-1) * boardSize.width]);
                    dstPoint.push_back(cv::Point2f(objectPoints[0][(boardSize.height-1) * boardSize.width].x,
                                                   objectPoints[0][(boardSize.height-1) * boardSize.width].y));
                    
                    camBeamerPerspTrans = cv::getPerspectiveTransform(inPoint, dstPoint);
                    
                    std::cout << "Beamer successfully calibrated to camera, repro error: " << rms << std::endl;
                    std::cout << "rotate mat: " << camBeamerRvecs[0] << std::endl;
                    std::cout << "translate mat: " << camBeamerTvecs[0] << std::endl;
                    std::cout << camBeamerMat << std::endl;
                    
                    saveMatr(camBeamerMatrPath, camBeamerMat, camBeamerDistCoeffs,
                             camBeamerRvecs, camBeamerTvecs, rms, camBeamerPerspTrans);
                    
                    camBeamerTransMatr = rtMatToGlm(kin->getColorWidth(), kin->getColorHeight(),
                                                    camBeamerRvecs[0], camBeamerTvecs[0]);

                    gotCamBeamerMapping = true;
                }
            }
        }
    }

    
    
    void SNTMapping::getRealWorldCoord(glm::vec3& inCoord)
    {
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
    }

    

    glm::mat4 SNTMapping::rtMatToGlm(int imgW, int imgH, cv::Mat& rVec, cv::Mat& tVec,
                                     bool norm, bool inv)
    {
        glm::mat4 outM = glm::mat4(1.f);
        glm::vec3 transV;
        
        if(norm)
            transV = glm::vec3((float)tVec.at<double>(0) / (float)imgW,
                               (float)tVec.at<double>(1) / (float)imgH,
                               (float)tVec.at<double>(2) / (float)imgH * 2.f);
        else
            transV = glm::vec3((float)tVec.at<double>(0),
                               (float)tVec.at<double>(1),
                               (float)tVec.at<double>(2));
        transV.z *= -1.f;
        
        cout << "transVec: " << glm::to_string(transV) << endl;
        
        outM = glm::translate(outM, transV);
        
        if(inv)
        {
            outM = glm::rotate(outM, (float)rVec.at<double>(0) * -1.f, glm::vec3(1.f, 0.f, 0.f));
            outM = glm::rotate(outM, (float)rVec.at<double>(1) * -1.f, glm::vec3(0.f, 1.f, 0.f));
            outM = glm::rotate(outM, (float)rVec.at<double>(2) * -1.f, glm::vec3(0.f, 0.f, 1.f));
        } else
        {
            outM = glm::rotate(outM, (float)rVec.at<double>(0), glm::vec3(1.f, 0.f, 0.f));
            outM = glm::rotate(outM, (float)rVec.at<double>(1), glm::vec3(0.f, 1.f, 0.f));
            outM = glm::rotate(outM, (float)rVec.at<double>(2), glm::vec3(0.f, 0.f, 1.f));
        }
        
        return outM;
    }

    

    void SNTMapping::saveMatr(std::string filename, cv::Mat& _cameraMatrix,
                              cv::Mat& _distCoeffs, std::vector<cv::Mat>& _rvecs,
                              std::vector<cv::Mat>& _tvecs, float totalAvgErr,
                              cv::Mat& camBeamerPerspTrans)
    {
        FileStorage fs( filename, FileStorage::WRITE );
        
        cout << "saving matrix to " << filename << endl;
        
        time_t tt;
        time( &tt );
        struct tm *t2 = localtime( &tt );
        char buf[1024];
        strftime( buf, sizeof(buf)-1, "%c", t2 );
        
        fs << "image_width" << imageSize.width;
        fs << "image_height" << imageSize.height;
        fs << "board_width" << boardSize.width;
        fs << "board_height" << boardSize.height;
        
        fs << "perspective_matrix" << cameraMatrix;
        fs << "distortion_coefficients" << distCoeffs;
        fs << "perp_trans" << camBeamerPerspTrans;

        fs << "rvec" << _rvecs[0];
        fs << "tvec" << _tvecs[0];
        
        fs << "avg_reprojection_error" << totalAvgErr;
        
        if( !_rvecs.empty() && !_tvecs.empty() )
        {
            CV_Assert(_rvecs[0].type() == _tvecs[0].type());
            Mat bigmat((int)_rvecs.size(), 6, _rvecs[0].type());
            for( int i = 0; i < (int)_rvecs.size(); i++ )
            {
                Mat r = bigmat(Range(i, i+1), Range(0,3));
                Mat t = bigmat(Range(i, i+1), Range(3,6));
                
                CV_Assert(_rvecs[i].rows == 3 && _rvecs[i].cols == 1);
                CV_Assert(_tvecs[i].rows == 3 && _tvecs[i].cols == 1);
                //*.t() is MatExpr (not Mat) so we can use assignment operator
                r = _rvecs[i].t();
                t = _tvecs[i].t();
            }
            //cvWriteComment( *fs, "a set of 6-tuples (rotation vector + translation vector) for each view", 0 );
            fs << "extrinsic_parameters" << bigmat;
        }
    }
    
    
    
    void SNTMapping::update(double time, double dt)
    {}
    
    

    void SNTMapping::onKey(int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS)
        {
            switch (key)
            {
                case GLFW_KEY_H:
                    depthToColorOffset.y += 0.01f;
                    std::cout << depthToColorOffset.y << std::endl;
                    break;
                case GLFW_KEY_N:
                    depthToColorOffset.y -= 0.01f;
                    std::cout << depthToColorOffset.y << std::endl;
                    break;
            }
        }
    }
    
    
    
    void SNTMapping::onCursor(double xpos, double ypos)
    {
        mouseX = static_cast<float>(xpos / static_cast<double>(scd->screenWidth));
        mouseY = static_cast<float>(ypos / static_cast<double>(scd->screenHeight));
    }
  
    

    void SNTMapping::onMouseButton(int button, int action, int mods)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        {
            glm::vec3 worldCoord;
            const openni::DepthPixel* pDepthPix = (const openni::DepthPixel*)kin->getDepthFrame()->getData();
            int rowSize = kin->getDepthFrame()->getStrideInBytes() / sizeof(openni::DepthPixel);
            int intX = static_cast<int>(mouseX * static_cast<float>(kin->getDepthWidth()));
            int intY = static_cast<int>(mouseY * static_cast<float>(kin->getDepthHeight()));
            pDepthPix += rowSize * intY;
            pDepthPix += intY;
            
            openni::CoordinateConverter::convertDepthToWorld(*kin->getDepthStream(),
                                                             intX,
                                                             intY,
                                                             *pDepthPix,
                                                             &worldCoord[0],
                                                             &worldCoord[1],
                                                             &worldCoord[2]);
            
            cout << glm::to_string(worldCoord) << endl;
        }
    }
}
