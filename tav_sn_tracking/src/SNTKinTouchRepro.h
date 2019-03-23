//
// SNTKinTouchRepro.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <OpenNI.h>

#include <GeoPrimitives/Quad.h>
#include <GLUtils/GLSL/FastBlur.h>
#include <GLUtils/FBO.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/PropoImage.h>
#include <GLUtils/TextureManager.h>
#include <GUI/GUIImgSlider.h>
#include <GUI/Widget.h>
#include <OpenCvUtils/CvKalmanFilter.h>
#include <KinectInput/KinectReproTools.h>
#include <Shaders/Shaders.h>
#include <Median.h>
#include <SceneNode.h>
#include <TrackObj.h>

namespace tav
{
    class SNTKinTouchRepro : public SceneNode
    {
    public:
        SNTKinTouchRepro(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTKinTouchRepro();
    
        void initKinDependent();
        void buildWidget();
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        
        void procBlobs(double time, double dt);
        void procGestures(double time);
        
        void myNanoSleep(uint32_t ns);
        
        void swipeImg();
        void clickBut();
        void sendOsCmd();
        
    
    

        void update(double time, double dt);
        void onMouseButton(GLFWwindow* window, int button, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);
        void onKey(int key, int scancode, int action, int mods);
        
    private:
        Quad*                           quad;
        Quad*                           rawQuad;
        Quad*                           whiteQuad;
        
        GWindowManager*                 winMan;
        std::string*                    dataPath;
        
        Shaders*                        shdr;
        Shaders*                        depthRotShdr;
        ShaderCollector*				shCol;

        KinectInput*                    kin;
        TextureManager*                 debugTex;
        
        cv::Mat                         view, depth, rview;
        cv::Mat                         checkWarp, checkWarpDepth;
        cv::Mat                         thresholded, thresh8;
        cv::Mat                         im_with_keypoints;
        
        std::vector<cv::Point2f>        pointbuf;
        
#ifdef __APPLE__
        std::vector< glm::vec4 >        kpCoords;
        
        std::map<float, int>            dists;
        std::map<float, int>            allDistMap;
        
        bool*                           hasNewVal;
        std::vector<int>*               ids;
        std::vector<float>*             pos;
        std::vector<float>*             speeds;
        std::vector<float>*             accels;
        //         TrackObj**                      trackObjs;
#endif
        cv::Ptr<cv::SimpleBlobDetector> detector;
        std::vector<TrackObj>           trackObjs;
        KinectReproTools*               kinRepro;
        
        GLuint                          depthTexNr;
        GLuint                          colorTexNr;
        GLuint                          showTexNr;
        
        bool                            isInited = false;
        bool                            saved = false;
        bool                            clicked = false;
        bool                            manMirror = false;
        bool                            hFlip = false;
        bool                            mouseDrag = false;
        
        
        
        int                             kinDeviceNr;
        int                             frameNr = -1;
        int                             colFrameNr = -1;
        int                             show;
        
        int                             gotCamToBeamer = 0;
        int                             getNrCheckSamples;
        int                             maxNrTrackObjs;
        unsigned int                    widgetWinInd;
        
        float                           depthSafety;
        float                           touchDepth;
        
        float                           gestureScaleWidth;
        float                           gestureScaleHeight;
        
        glm::vec2                       kinDepthSize;
        glm::vec2                       actMousePos;
        
        KinectReproTools::mode          actMode;
        std::string                     calibFile;
        
        Widget*                         widget;
        std::vector<std::string>        imgSlidTexs;
        
        Shaders*                        blendShader;
    };
}
