//
// SNTAugChessb2.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <opencv2/core.hpp>
#include <OpenNI.h>

#include <headers/gl_header.h>
//#include <headers.h>
#include <GeoPrimitives/Line.h>
#include <GeoPrimitives/Quad.h>
#include <GLUtils/GLMCamera.h>
#include <GLUtils/TextureManager.h>
#include <GLUtils/PropoImage.h>
#include <Shaders/Shaders.h>
#include <KinectInput/KinectInput.h>
#include <OpenCvUtils/CvKalmanFilter.h>

#define GLM_FORCE_RADIANS

#include <SceneNode.h>

namespace tav
{
    class SNTAugChessb2 : public SceneNode
    {
    public:
        enum drawMode { CHECK_KIN_OVERLAY=0, CHESS_TO_CAM=1, CHECK_CAM_TO_BEAMER=2, SAVE_CAM_TO_BEAMER=3,
            CHECK_KIN_UNWARP=4, CHECK_BEAMER_TO_OBJ=5, SAVE_BEAMER_TO_OBJ=6 };
        enum beamerModel { ZOOM=0, WIDE=1, ULTRA_WIDE=2 };

        typedef struct
        {
            bool        invertMatr;
            cv::Size    imgSize;    // image size of the camera input
            cv::Size    depthImgSize;
            cv::Size    boardSize;
            int         nrSamples;
            float       beamerAspectRatio;
            float       beamerFovX;
            float       beamerFovY;
            float       beamerLowEdgeAngle;
            float       beamerThrowRatio;
            float       beamerWallDist;
            float       camAspectRatio;
            float       camFovX;
            float       camFovY;
            float       camWallRectReal;
            float       chessRealWidth;
            
            glm::mat4   chessRotXY;
            glm::mat4   chessRotZ;
            glm::mat4   chessTrans;

            glm::mat4   camBeamerRotXY;
            glm::mat4   camBeamerRotZ;
            glm::mat4   camBeamerTrans;

            glm::vec3   beamerRealMid;
            glm::vec3   camBeamerRealOffs;
            glm::vec3   camBeamerNormal;
            glm::vec3   chessRealMid;
            glm::vec3   chessNormal;
            
            glm::vec2   camRealWorldXYScale;

            beamerModel beamModel;
            cv::Mat     cameraMatrix;
            cv::Mat     distCoeffs;
            cv::Mat     camBeamerPerspTrans;
            cv::Mat     camBeamerInvPerspTrans;
        } kpCalibData;
        
        SNTAugChessb2(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTAugChessb2();
    
        void initKinDependent();
        void update(double time, double dt);

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void drawChessToCam(double time, double dt);
        void drawKinOverlay();
        void drawCamToBeamer();
        void drawCamToBeamerInv();
        void drawKinUnwrap();
        void drawBeamerToObj();
        
        void getRealWorldKinBeamerOffs(kpCalibData& kp);
        void getRealWorldCoord(glm::vec3& inCoord, glm::vec2& _kinRealWorldXYScale);
        void getRealWorldCoordUnwarp(glm::vec3& inCoord, glm::vec2& _kinRealWorldXYScale,
                                     cv::Mat& perspTrans, bool getWarpedXY);
        glm::mat4 rtMatToGlm(int imgW, int imgH, cv::Mat& rVec, cv::Mat& tVec, bool inv=false);
        glm::mat4 getBeamerFromCv(cv::Mat& camMatr, float calibWidth, float calibHeight, float znear,
                                float zfar, bool yUp=true);
        cv::Mat getPerspTrans(std::vector<cv::Point2f>& inPoints);
        glm::mat4 getRotRealToGL(kpCalibData& kpData);
        glm::mat4 getRotRealToCamToGL(kpCalibData& kpData);
        glm::vec3 getChessMidPoint(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp);
        glm::mat4 calcChessRotXY(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp);
        glm::mat4 calcChessRotZ(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp);
        glm::mat4 calcChessTrans(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp, bool normalize=true);
        void loadCamToBeamerCalib(cv::FileStorage& fs, kpCalibData& kp);
        void saveCamToBeamerCalib(std::string _filename, kpCalibData& kp);
        cv::Mat glmToCvMat(glm::mat4& mat);
        glm::mat4 cvMatToGlm(cv::Mat& _mat);
        glm::mat4 cvMat33ToGlm(cv::Mat& _mat);

        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);
        void onMouseButton(GLFWwindow* window, int button, int action, int mods);
    private:
        KinectInput*    kin;
        Quad*           quad;
        Quad*           noFlipQuad;
        Quad*           whiteQuad;
        Quad*           kinQuad;
        
        Line*           xLine;
        Line*           yLine;
        
        ShaderCollector*				shCol;
        Shaders*        shdr;
        Shaders*        colShader;
        Shaders*        blendShader;

        TextureManager* kinTex;
        TextureManager* kinDepthTex;
        
        PropoImage*     chessBoard;
        PropoImage*     chessBoard2;
        PropoImage*     chessBoard3;
        
        CvKalmanFilter* kalmanRot;
        CvKalmanFilter* kalmanRotZ;
        CvKalmanFilter* kalmanTrans;
        GLuint          colorTexNr;
        
        kpCalibData     kpCalib;
        
        bool            isInited = false;
        bool            beamerUpsideDown;
        bool            invertCamBeamerMatr;
        bool            switchBeamerToObj = false;
        
        int             gotCamToBeamer = 0;
        int             gotBeamerToObj = 0;
        int             beamerToObjIntv;
        
        int             frameNr = -1;
        int             depthFrameNr = -1;
        int             getNrCheckSamples;
        
        float           beamerLowEdgeAngle;
        float           beamerThrowRatio;
        
        float           glChessBoardWidth;
        float           glChessBoardHeight;
        
        float           chessOffsX=0.f;
        float           chessOffsY=0.f;
        
        float           kinFovY;
        float           kinRealWorldXYScale;
        
        float           tBeamerDistRectReal = 1.f;
        float           oldBeamerDistRectReal = 800.f;
        
        float           realToNorm;
        float           realBoardWidth;
        float           coordNormFact;
        
        
        drawMode        actDrawMode;
                
        std::string     camBeamerMatrPath;
        
        cv::Mat         testChessFile;
        cv::Mat         map1, map2;
        cv::Mat         view, viewDepth, rview;
        cv::Mat         viewGray;
        cv::Mat         checkWarp, checkWarpDepth;
        
        GLMCamera       mapCam;

        std::vector<cv::Point2f> pointbuf;
        std::vector<glm::vec4> chessbXyPlane;

        glm::vec3       objNormal;

        glm::mat4       chessbMatr;
        glm::mat4       chessRotMatr;
        glm::mat4       camBeamerRotMatr;
        
        glm::mat4       objRotMatr;
        glm::mat4       objRotXYMatr;
        glm::mat4       objRotZMatr;
        glm::mat4       objTrans;
    };
}
