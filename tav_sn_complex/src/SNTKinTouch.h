//
// SNTKinTouch.h
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

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h> // order is important!! put after opencv
#include <unistd.h>
#endif

#include <KinectInput/KinectInput.h>

#include <Communication/OSC/OSCData.h>
#include <GeoPrimitives/Quad.h>
#include <GLUtils/GLSL/FastBlur.h>
#include <GLUtils/FBO.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/PropoImage.h>
#include <GLUtils/TextureManager.h>
#include <GUI/GUIImgSlider.h>
#include <GUI/Widget.h>
#include <OpenCvUtils/CvKalmanFilter.h>
#include <Shaders/Shaders.h>
#include <TrackObj.h>
#include <Median.h>
#include <SceneNode.h>

namespace tav
{
    class SNTKinTouch : public SceneNode
    {
    public:
        enum mode { CALIBRATE=0, CHECK_ROT=1, CHECK=2, USE=3, RAW_DEPTH=4, RAW_COLOR=5 };
        enum screenType { BEAMER=0, LED=1 };
        
        typedef struct
        {
            cv::Size    boardSize;
            cv::Size    imgSize;    // image size of the camera input
            cv::Size    depthImgSize;
            cv::Mat     camBeamerPerspTrans;
            glm::vec3   rotations;
            glm::vec3   camBeamerRots;
            glm::vec2   camFov;
            float       camAspectRatio;
            float       camWallDist;
            float       touchDepth;
            float       depthSafety;
            float       cropDown = -1.f;
            float       cropUp = 1.f;
            float       cropLeft = -1.f;
            float       cropRight = 1.f;
            float       threshZDeep;
        } ktCalibData;
        

        SNTKinTouch(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        virtual ~SNTKinTouch();
        
        void initKinDependent();
        Shaders* initDepthRotShdr();
        Shaders* initTexBlendShdr();

        void buildWidget();
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        
        void procBlobs(double time, double dt);
        void procGestures(double time);
        void perspUnwarp(glm::vec2& inPoint, glm::mat4& perspMatr);
        void rotDepthTex();
        void reCheckRot(ktCalibData& kp);
        
        glm::mat4 calcChessRotXYWall(std::vector<cv::Point2f> _pointbuf, ktCalibData& kp, cv::Size boardSize);
        glm::mat4 calcChessRotZ(std::vector<cv::Point2f> _pointbuf, ktCalibData& kp);
        float calcWallDist(ktCalibData& kp);
        void getKinRealWorldCoord(glm::vec3& inCoord, ktCalibData& kp);
        cv::Mat getPerspTrans(std::vector<cv::Point2f>& inPoints, float _chessWidth, float _chessHeight);
        glm::mat4 cvMat33ToGlm(cv::Mat& _mat);
        
        void loadCamToBeamerCalib(cv::FileStorage& fs, ktCalibData& kp);
        void saveCamToBeamerCalib(std::string _filename, ktCalibData& kp);
        
        void myNanoSleep(uint32_t ns);
        
        void swipeImg();
        void clickBut();
        void sendOsCmd();

        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
        
    private:
        Quad*                           quad;
        Quad*                           rawQuad;
        Quad*                           whiteQuad;
        
        Shaders*                        shdr;
        Shaders*                        stdCol;
        Shaders*                        depthRotShdr;
        ShaderCollector*                shCol;
        
        PropoImage*                     chessBoard;
        FBO*                            rotDepth;
        FastBlur*                       fastBlur;
        TextureManager*                 kinDepth;
        TextureManager*                 kinDepthDebug;
        TextureManager*                 kinColor;
        TextureManager*                 debugTex;
        
        GWindowManager*                 winMan;
        OSCData*                        osc;
        KinectInput*                    kin;
        std::string*                    dataPath;
        
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
        
        GLuint                          depthTexNr;
        GLuint                          colorTexNr;
        GLuint                          showTexNr;
        
        bool                            isInited = false;
        bool                            saved = false;
        bool                            clicked = false;
        bool                            manMirror = false;
        
        int                             frameNr = -1;
        int                             colFrameNr = -1;
        int                             show;
        
        
        int                             gotCamToBeamer = 0;
        int                             getNrCheckSamples;
        int                             maxNrTrackObjs;
        int                             kinDeviceNr;
        unsigned int                    widgetWinInd;
        
        float                           kinFovY;
        float                           kinFovX;
        float                           kinDepthW;
        float                           kinDepthH;
        float                           glChessBoardWidth;
        float                           glChessBoardHeight;
        
        // oscPar
        float                           rotXOffs=0;
        float                           rotYOffs=0;
        float							threshZDeep=0;
        float							touchDepth=0;
        float							cropLeft=0.f;
        float							cropRight=0.f;
        float							cropUp;
        float							cropDown;

        float                           depthSafety;
        
        float                           gestureScaleWidth;
        float                           gestureScaleHeight;
        
        Median<float>*                  camWallDist;
        Median<glm::vec3>*              rotations;
        glm::mat4                       perspDistMat;
        
        ktCalibData                     ktCalib;
        mode                            actMode;
        std::string                     calibFile;
        
#ifdef __APPLE__
        CGDisplayCount                  dspCount;
        CGDirectDisplayID*              displays;
#endif
        
        Widget*                         widget;
        std::vector<std::string>        imgSlidTexs;
        
        Shaders*                        blendShader;
        
        screenType						screenType;

    };
}
