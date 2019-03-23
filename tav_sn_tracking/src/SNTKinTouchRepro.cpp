//
// SNTKinTouchRepro.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTKinTouchRepro.h"

using namespace std;

namespace tav
{
    SNTKinTouchRepro::SNTKinTouchRepro(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs),
    actMode(KinectReproTools::CHECK_RAW_COLOR),
    depthSafety(56.f),
    touchDepth(20.f),
    maxNrTrackObjs(5),
    kinDeviceNr(0),
    manMirror(true), // true for installation upside down on the ceiling
    hFlip(false),
    widgetWinInd(0)
    {
    	kin = static_cast<KinectInput*>(scd->kin);
    	winMan = static_cast<GWindowManager*>(scd->winMan);
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        // add onKeyFunction to Window
        winMan->addKeyCallback(widgetWinInd,
                               [this](int key, int scancode, int action, int mods) {
                                        return this->onKey(key, scancode, action, mods); });
        
        quad = scd->stdQuad;
        quad->rotate(M_PI, 0.f, 0.f, 1.f);
        quad->rotate(M_PI, 0.f, 1.f, 0.f);
        
        rawQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
                           glm::vec3(0.f, 0.f, 1.f),
                           0.f, 0.f, 0.f, 1.f);        
        
        whiteQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
                             glm::vec3(0.f, 0.f, 1.f),
                             1.f, 1.f, 1.f, 1.f);
        
        calibFile = (*scd->dataPath)+("calib_cam/kinTouchRepro.yml");
        
        
        cv::SimpleBlobDetector::Params pDefaultBLOB;
        
        // This is default parameters for SimpleBlobDetector
        pDefaultBLOB.thresholdStep = 10;
        pDefaultBLOB.minThreshold = 10;
        pDefaultBLOB.maxThreshold = 220;
        pDefaultBLOB.minRepeatability = 2;
        pDefaultBLOB.minDistBetweenBlobs = 10;
        pDefaultBLOB.filterByColor = false;
        pDefaultBLOB.blobColor = 0;
        pDefaultBLOB.filterByArea = true;
        pDefaultBLOB.minArea = 25; // 35
        pDefaultBLOB.maxArea = 5000;
        
        pDefaultBLOB.filterByCircularity = false;
        pDefaultBLOB.minCircularity = 0.9f;
        pDefaultBLOB.maxCircularity = (float)1e37;
        
        pDefaultBLOB.filterByInertia = false;
        pDefaultBLOB.minInertiaRatio = 0.1f;
        pDefaultBLOB.maxInertiaRatio = (float)1e37;
        
        pDefaultBLOB.filterByConvexity = false;
        pDefaultBLOB.minConvexity = 0.95f;
        pDefaultBLOB.maxConvexity = (float)1e37;
        
        detector = cv::SimpleBlobDetector::create(pDefaultBLOB);
        
        /*
         trackObjs = new TrackObj*[maxNrTrackObjs];
         hasNewVal = new bool[maxNrTrackObjs];
         for (int i=0;i<maxNrTrackObjs;i++) {
         trackObjs[i] = new TrackObj(i);
         hasNewVal[i] = false;
         }
         
         
         ids = new vector<int>();
         pos = new vector<float>();
         speeds = new vector<float>();
         accels = new vector<float>();
         */
        
        buildWidget();
        
        actMousePos = glm::vec2(0.f, 0.f);
        show = 0;
    }
    

    
    void SNTKinTouchRepro::initKinDependent()
    {
        kin->setImageRegistration(true);
        //kin->setMirror(false); // geht nicht bei depth stream, warum auch immer..
        
        kinDepthSize = glm::vec2(static_cast<float>(kin->getDepthWidth(kinDeviceNr)),
                                 static_cast<float>(kin->getDepthHeight(kinDeviceNr)));
        
        kinRepro = new KinectReproTools(winMan, kin, shCol, scd->screenWidth,
                                        scd->screenHeight, *(scd->dataPath), kinDeviceNr);
        
        kinRepro->noUnwarp();
        kinRepro->loadCalib(calibFile);
        kinRepro->setMode(actMode);
        kinRepro->setHFlip(hFlip);
    }
    
    
    
    void SNTKinTouchRepro::buildWidget()
    {
        // build widget
        widget = new Widget();
        widget->setPos(-1.f, -1.f);
        widget->setSize(2.f, 2.f);
        widget->setBackColor(0.f, 0.f, 0.f, 1.f);
        //widget->setBackTex((*scd->dataPath)+"textures/blue-abstract-background.JPG");
        
        // add image slider
        imgSlidTexs.push_back((*(scd->dataPath))+"textures/House.jpg");
        
        
        // normalize to height proprtional
        float propoSlides = 1769.f / 995.f;
        float screenAspect = float(scd->screenHeight) / float(scd->screenWidth);
        float propoHeight = 2.f / screenAspect / propoSlides;
        widget->add( new GUIImgSlider(glm::vec2(-1.f, (2.f - propoHeight) * 0.5f -1.f), glm::vec2(2.f, propoHeight) ) );
        
        
        widget->getLastGuiObj()->setBackColor(0.f, 0.f, 0.f, 1.f);
        widget->getLastGuiObj()->setColor(0.f, 0.f, 0.f, 1.f);
        widget->getLastGuiObj()->setTextures(imgSlidTexs);
        widget->getLastGuiObj()->setAction(SWIPE_RIGHT, [this]() { return this->swipeImg(); });
        widget->getLastGuiObj()->setActionAnim(SWIPE_RIGHT, GUI_MOVE_RIGHT);
        widget->getLastGuiObj()->setActionAnimDur(GUI_MOVE_RIGHT, 1.0);
        widget->getLastGuiObj()->setAction(SWIPE_LEFT, [this]() { return this->swipeImg(); });
        widget->getLastGuiObj()->setActionAnim(SWIPE_LEFT, GUI_MOVE_LEFT);
        widget->getLastGuiObj()->setActionAnimDur(GUI_MOVE_LEFT, 1.0);
        
        widget->getLastGuiObj()->setAction(SWIPE_UP, [this]() { return this->swipeImg(); });
        widget->getLastGuiObj()->setActionAnim(SWIPE_UP, GUI_MOVE_UP);
        widget->getLastGuiObj()->setActionAnimDur(GUI_MOVE_UP, 1.0);
        widget->getLastGuiObj()->setAction(SWIPE_DOWN, [this]() { return this->swipeImg(); });
        widget->getLastGuiObj()->setActionAnim(SWIPE_DOWN, GUI_MOVE_DOWN);
        widget->getLastGuiObj()->setActionAnimDur(GUI_MOVE_DOWN, 1.0);
        
        // assign to first gwindow
        winMan->addWidget(widgetWinInd, widget, shCol);
    }
    
    
    
    void SNTKinTouchRepro::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
    	/*
        if (kin->isReady())
        {
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            
            switch(actMode)
            {
                case KinectReproTools::CHECK_RAW_DEPTH :
                    kinRepro->drawCheckDepth();
                    break;
                case KinectReproTools::CHECK_RAW_COLOR :
                    kinRepro->drawCheckColor();
                    break;
                case KinectReproTools::CALIB_ROT_WIN:
                    kinRepro->drawCalibWin();
                    break;
                case KinectReproTools::CHECK_ROT_WARP:
                    kinRepro->drawCheckRotWarp();
                    //printf("draw depth \n");
                    break;
                case KinectReproTools::CALIB_ROT_WARP:
                    kinRepro->drawFoundChess();
                    break;
                default:
                    break;
            }
        }
        */
    }
    
    
    
    void SNTKinTouchRepro::update(double time, double dt)
    {
        if (!isInited)
        {
            initKinDependent();
            isInited = true;
        }
        
        if (isInited)
        {
            kinRepro->update();

            switch(actMode)
            {
                case KinectReproTools::CHECK_ROT_WARP :
//                    kinRepro->setThreshZNear(kinRepro->getRCalibData()->distKinObj - osc->alpha * 500.f);
//                    kinRepro->setThreshZDeep(kinRepro->getCalibData()->distKinObj - osc->feedback * 100.f);
                    break;
                case KinectReproTools::USE_ROT_WARP :
                    kin->uploadColorImg(kinDeviceNr);
                    break;
                default:
                    break;
            }
            
            frameNr++;
        }
    }
    
    
    
    void SNTKinTouchRepro::procGestures(double time)
    {
        // loop through all found (active) trackpoints
        for(std::vector<TrackObj>::iterator it=trackObjs.begin(); it!=trackObjs.end(); ++it)
        {
            float posX = (*it).getPos().x * float(winMan->getMonitorWidth(0));
            float posY = ((*it).getPos().y) * float(winMan->getMonitorHeight(0));
            
            if( (*it).isActive() )
            {
                (*it).accumVel(true);
                
                if((*it).onTimeDelta(time) > 0.02f && !clicked)
                {
                    winMan->setWidgetCmd(widgetWinInd, posX - float(scd->screenWidth), posY, LEFT_CLICK);
                    clicked = true;
                }
                
                /*
                 
                 //cout << "vel " <<  glm::length((*it).getMedVel()) << endl;
                 
                 //if ((*it).onTimeDelta(time) > 0.3f && glm::length((*it).getAccumVel()) > 0.2f)
                 
                 if ((*it).onTimeDelta(time) > 0.3f && glm::length((*it).getMedVel()) > 0.07f && !(*it).getUse())
                 {
                 (*it).setUse(true);
                 float angle = std::atan2((*it).getMedVel().y, (*it).getMedVel().x);
                 //                    float angle = std::atan2((*it).getAccumVel().y, (*it).getAccumVel().x);
                 
                 //                        cout << "angle: " << angle << endl;
                 //                        cout << "std::fabs(angle - M_PI): " << std::fabs(angle - M_PI) << endl;
                 //                        cout << "std::fabs(angle - M_PI_2): " << std::fabs(angle - M_PI_2) << endl;
                 
                 //cout << "getAccumVel length: " << glm::length((*it).getAccumVel()) << endl;
                 //cout << "delta: " << (*it).onTimeDelta(time) << endl;
                 
                 
                 if (std::fabs(M_PI_2 + angle) < M_PI_4)
                 {
                 cout << "swipe up" << endl;
                 taMods->rootWidget->setCmd(posX - 1280.f, posY, SWIPE_UP);
                 } else if (std::fabs(angle - M_PI_2) < M_PI_4)
                 {
                 cout << "swipe down" << endl;
                 taMods->rootWidget->setCmd(posX - 1280.f, posY, SWIPE_DOWN);
                 
                 } else if ((M_PI - std::fabs(angle)) < M_PI_4)
                 {
                 cout << "swipe left" << endl;
                 taMods->rootWidget->setCmd(posX - 1280.f, posY, SWIPE_LEFT);
                 
                 } else if (std::fabs(angle) < M_PI_4)
                 {
                 cout << "swipe right" << endl;
                 taMods->rootWidget->setCmd(posX - 1280.f, posY, SWIPE_RIGHT);
                 }
                 
                 cout << endl;
                 }
                 */
            } else
            {
                
                // if inactive
                if( (*it).wasActive() )
                {
                    
                    if ((*it).onTimeDelta(time) > 0.3f && glm::length((*it).getAccumVel()) > 0.1f)
                    {
                        //(*it).setUse(true);
                        float angle = std::atan2((*it).getAccumVel().y, (*it).getAccumVel().x);
                        
                        //cout << "angle: " << glm::to_string((*it).getAccumVel()) << endl;
                        // cout << "std::fabs(angle - M_PI): " << std::fabs(angle - M_PI) << endl;
                        // cout << "std::fabs(angle - M_PI_2): " << std::fabs(angle - M_PI_2) << endl;
                        
                        //cout << "getAccumVel length: " << glm::length((*it).getAccumVel()) << endl;
                        //cout << "delta: " << (*it).onTimeDelta(time) << endl;
                        
                        
                        if (std::fabs(M_PI_2 + angle) < M_PI_4)
                        {
                            cout << "swipe up" << endl;
                            //                            taMods->rootWidget->setCmd(posX - float(scd->screenWidth), posY, SWIPE_UP);
                            // taMods->rootWidget->setCmd(posX - float(scd->screenWidth), posY, SWIPE_RIGHT);
                            
                        } else if (std::fabs(angle - M_PI_2) < M_PI_4)
                        {
                            //cout << "swipe down" << endl;
                            // taMods->rootWidget->setCmd(posX - 1280.f, posY, SWIPE_DOWN);
                            
                        } else if ((M_PI - std::fabs(angle)) < M_PI_4)
                        {
                            cout << "swipe left" << endl;
                            //taMods->rootWidget->setCmd(posX - float(scd->screenWidth), posY, SWIPE_LEFT);
                            
                        } else if (std::fabs(angle) < M_PI_4)
                        {
                            cout << "swipe right" << endl;
                            // taMods->rootWidget->setCmd(posX - float(scd->screenWidth), posY, SWIPE_RIGHT);
                        }
                        
                        cout << endl;
                    }
                    
                    (*it).accumVel(false);
                    (*it).setUse(false);
                    clicked = false;
                }
            }
        }
    }
    
    
    
    void SNTKinTouchRepro::procBlobs(double time, double dt)
    {
        glm::vec2 actPoint;
        
        // Detect blobs.
        std::vector<cv::KeyPoint> keypoints;
        //        detector->detect(thresh8, keypoints);
        detector->detect(checkWarpDepth, keypoints);
        
        // dirty test only with the first value
        if ( static_cast<int>(keypoints.size()) > 0 )
        {
            if ( static_cast<int>(trackObjs.size()) == 0 )
            {
                trackObjs.push_back( TrackObj() );
                trackObjs.back().setOn(time);
                
            } else
            {
                if(!trackObjs[0].isActive()) trackObjs[0].setOn(time);
                trackObjs[0].predict();
                
                // perspective undistortion
                actPoint = glm::vec2(keypoints[0].pt.x, keypoints[0].pt.y);
                //perspUnwarp(actPoint, perspDistMat);
                
                // normalize from 0 - 1
                if(!manMirror)
                    trackObjs[0].update(actPoint.x / kinDepthSize.x,
                                        actPoint.y / kinDepthSize.y,
                                        0.f, dt);
                else
                    trackObjs[0].update(actPoint.x / kinDepthSize.x,
                                        1.0 - (actPoint.y / kinDepthSize.y),
                                        0.f, dt);
                
            }
        } else
        {
            //cout << "keypoints.size(): " << keypoints.size() << endl;
            
            // to avoid false off-sets only take the off if it was send continously sent for a specific time
            for(std::vector<TrackObj>::iterator it=trackObjs.begin(); it!=trackObjs.end(); ++it)
                (*it).requestOff(time, dt);
        }
        
        
        /*
         // predict new results
         for (int i=0;i<maxNrTrackObjs;i++) trackObjs[i]->predict();
         
         kpCoords.clear(); allDistMap.clear();
         std::vector< std::map<float, int> > allDists;
         
         // loop through the found points
         for(int i=0;i<int(keypoints.size());i++)
         {
         float x = keypoints[i].pt.x / kin->getDepthWidth() -0.5f;
         float y = 0.5f - keypoints[i].pt.y / kin->getDepthHeight();
         
         kpCoords.push_back( glm::vec4(x, y, 0.f, 0.f) );
         
         dists.clear();
         
         // loop through all present trackPoints and calculate distances to the actual point
         for(int j=0;j<maxNrTrackObjs;j++)
         dists[ trackObjs[j]->getDist(x, y, 0.f) ] = j;
         
         allDists.push_back(dists);
         
         // save the lowest result in a map
         allDistMap[ dists.begin()->first ] = i;
         }
         
         
         // gehe alle resultate durch und wirf das zugeordnete TrackObj aus allen anderen Maps raus
         ids->clear(); pos->clear(); speeds->clear(); accels->clear();
         for (std::map<float,int>::iterator it=allDistMap.begin(); it!=allDistMap.end(); ++it)
         {
         int kpNr = (*it).second;
         int trackObjNr = allDists[kpNr].begin()->second;
         
         trackObjs[trackObjNr]->update( kpCoords[kpNr].x, kpCoords[kpNr].y, kpCoords[kpNr].z, dt );
         
         ids->push_back(trackObjNr);
         
         pos->push_back(kpCoords[kpNr].x);
         pos->push_back(kpCoords[kpNr].y);
         
         speeds->push_back( trackObjs[trackObjNr]->getVel(0) );
         speeds->push_back( trackObjs[trackObjNr]->getVel(1) );
         
         accels->push_back( trackObjs[trackObjNr]->getAccel(0) );
         accels->push_back( trackObjs[trackObjNr]->getAccel(1) );
         
         for (int i=0; i<allDists.size(); i++)
         {
         // look for the entry with the same trackobj nr in the specific dist map
         bool found = false;
         std::map<float,int>::iterator killThis;
         for (std::map<float,int>::iterator dIt=allDists[i].begin(); dIt!= allDists[i].end(); dIt++)
         {
         if ( (*dIt).second == trackObjNr )
         {
         found = true; killThis = dIt;
         }
         }
         if (found) allDists[i].erase( killThis );
         }
         }
         */
        
        // Draw detected blobs as red circles.
        // DrawMatchesFlags::DRAW_RICH_KEYPOINTS flag ensures the size of the circle corresponds to the size of blob
        //        cv::drawKeypoints(thresh8, keypoints, im_with_keypoints,
        //                          cv::Scalar(255,0,255),
        //                          cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        
        cv::drawKeypoints(checkWarpDepth, keypoints, im_with_keypoints,
                          cv::Scalar(255,0,255),
                          cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        
    }
    
    
    
    void SNTKinTouchRepro::myNanoSleep(uint32_t ns)
    {
        struct timespec tim;
        tim.tv_sec = 0;
        tim.tv_nsec = (long)ns;
        nanosleep(&tim, NULL);
    }
    
    
    
    void SNTKinTouchRepro::swipeImg()
    {}
    
    
    
    void SNTKinTouchRepro::clickBut()
    {}
    
    

    void SNTKinTouchRepro::onMouseButton(GLFWwindow* window, int button, int action, int mods)
    {
        if (isInited)
        {
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
            {
                mouseDrag = true;
                kinRepro->setCalibDragInit(actMousePos.x, actMousePos.y);
            }
            
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
            {
                mouseDrag = false;
            }
        }
    }

    
    
    void SNTKinTouchRepro::onCursor(GLFWwindow* window, double xpos, double ypos)
    {
        actMousePos.x = xpos / static_cast<double>(scd->screenWidth);
        actMousePos.y = ypos / static_cast<double>(scd->screenHeight);
        
        if(mouseDrag)
            kinRepro->dragCalibWin(actMousePos.x, actMousePos.y);
    }

    
    
    void SNTKinTouchRepro::onKey(int key, int scancode, int action, int mods)
    {
        // trapez korrektur schnell und schmutzig
        if (action == GLFW_PRESS)
        {
            if(mods == GLFW_MOD_SHIFT)
            {
                switch (key)
                {
                    case GLFW_KEY_1 :
                        actMode = KinectReproTools::CHECK_RAW_DEPTH;
                        printf("CHECK_RAW_DEPTH \n");
                        break;
                    case GLFW_KEY_2 :
                        actMode = KinectReproTools::CHECK_RAW_COLOR;
                        printf("CHECK_RAW_COLOR \n");
                        break;
                    case GLFW_KEY_3 :
                        actMode = KinectReproTools::CALIB_ROT_WIN;
                        printf("CALIB_ROT_WIN \n");
                        break;
                    case GLFW_KEY_4 :
                        actMode = KinectReproTools::CALIB_ROT_WARP;
                        printf("CALIB_ROT_WARP \n");
                        break;
                    case GLFW_KEY_5 :
                        actMode = KinectReproTools::CHECK_ROT_WARP;
                        printf("CHECK_ROT_WARP \n");
                        break;
                    case GLFW_KEY_6 :
                        actMode = KinectReproTools::CHECK_ROT_WARP_LOADED;
                        kinRepro->loadCalib(calibFile);
                        printf("CHECK_ROT_WARP_LOADED \n");
                        break;
                    case GLFW_KEY_7 : actMode = KinectReproTools::USE_ROT_WARP;
                        printf("USE_ROT_WARP \n");
                        break;
                    case GLFW_KEY_S :
                        printf("SAVE \n");
                        kinRepro->saveCalib(calibFile);
                        break;
                }
                
                kinRepro->setMode(actMode);
            } else
            {
                switch (key)
                {
                    case GLFW_KEY_UP : winMan->setWidgetCmd(widgetWinInd, 320.f, 240.f, SWIPE_UP); break;
                    case GLFW_KEY_LEFT : winMan->setWidgetCmd(widgetWinInd, 320.f, 240.f, SWIPE_LEFT); break;
                    case GLFW_KEY_RIGHT : winMan->setWidgetCmd(widgetWinInd, 320.f, 240.f, SWIPE_RIGHT); break;
                }
            }
        }
    }
    

    
    SNTKinTouchRepro::~SNTKinTouchRepro()
    {
        delete quad;
    }
    
}
