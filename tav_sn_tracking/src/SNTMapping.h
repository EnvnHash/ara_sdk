//
// SNTMapping.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <opencv2/core.hpp>

//#include <headers.h>
#include <headers/gl_header.h>
#include <GeoPrimitives/Quad.h>
#include <GLUtils/GLMCamera.h>
#include <GLUtils/PropoImage.h>
#include <GLUtils/TextureManager.h>
#include <KinectInput/KinectInput.h>
#include <Shaders/ShaderCollector.h>
#include <Shaders/Shaders.h>

#include <SceneNode.h>

#include <GLFW/glfw3.h>
namespace tav
{
    class SNTMapping : public SceneNode
    {
    public:
        SNTMapping(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTMapping();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void getCamBeamerMapping(double time);
        void updateKinectTex();
        void getRealWorldCoord(glm::vec3& inCoord);
        glm::mat4 rtMatToGlm(int imgW, int imgH, cv::Mat& rVec, cv::Mat& tVec, bool norm=true, bool inv=true);
        void saveMatr(std::string filename, cv::Mat& _cameraMatrix,
                      cv::Mat& _distCoeffs, std::vector<cv::Mat>& _rvecs,
                      std::vector<cv::Mat>& _tvecs, float totalAvgErr,
                      cv::Mat& camBeamerPerspTrans);
    
    
        void update(double time, double dt);

        void onKey(int key, int scancode, int action, int mods);
        void onCursor(double xpos, double ypos);
        void onMouseButton(int button, int action, int mods);
    private:
        KinectInput*    kin;
        Quad*           quad;
        Quad*           noFlipQuad;
        Quad*           whiteQuad;
        Quad*           kinQuad;
        Quad*           kinDepthQuad;

        ShaderCollector* shCol;
        Shaders*        shdr;
        Shaders*        blendShader;
        
        TextureManager* kinTex;
        TextureManager* kinDepthTex;
        PropoImage*     chessBoard;
        
        GLuint          depthTexNr;
        GLuint          colorTexNr;
        
        bool            isInited = false;
        bool            useFishEye = false;
        bool            gotCamBeamerMapping;
        bool            calibCamToBeamer;
        bool            beamerUpsideDown;
        bool            gotPose;
        bool            useHisto = false;
        
        int             frameNr = -1;
        int             depthFrameNr = -1;
        int             nrBeamerToCamSnapshots;
        int             snapshotCtr=0;
        int             oldFrameSnapNr=-1;
        
        float           chessBoardWidth;
        float           chessBoardHeight;
        float           beamerThrowRatio;
        float           mouseX;
        float           mouseY;
        float           camRotX;
        float           realVirtualFact;
        float           fovX, fovY, fovD;
        float           focalLength;
        
        double          kinAspectRatio;
        
        cv::Point2d     principalPoint;
        
        glm::vec3       depthToColorOffset;
        
        std::string     camBeamerMatrPath;
        
        cv::Size        imageSize;
        cv::Size        boardSize;
        
        cv::Mat         cameraMatrix;
        cv::Mat         distCoeffs;
        cv::Mat         map1, map2;
        cv::Mat         view, rview;

        cv::Mat         beamerCamMatrix;
        cv::Mat         beamerCamDistCoeffs;

        cv::Mat         dCameraMatrix;
        cv::Mat         dDistCoeffs;
        cv::Mat         dMap1, dMap2;
        cv::Mat         dView, dRview;
        
        cv::Mat         viewGray;
        cv::Mat         camBeamerMat;
        cv::Mat         camBeamerDistCoeffs;
        cv::Mat         checkWarp;
        std::vector<cv::Mat> camBeamerRvecs;
        std::vector<cv::Mat> camBeamerTvecs;
        cv::Mat         camBeamerPerspTrans;
        glm::mat4       camBeamerTransMatr;
        
        cv::Mat         rvec;
        cv::Mat         tvec;
        
        GLMCamera*      mapCam;

        std::vector<std::vector<cv::Point3f> > objectPoints;
        std::vector<std::vector<cv::Point2f> > imagePoints;
        std::vector<cv::Point2f> pointbuf;
    };
}
