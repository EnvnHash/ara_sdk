//
//  SNGLSLFluidTest.h
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__SNKinectPainting2__
#define __Tav_App__SNKinectPainting2__

#pragma once

#include <iostream>
#include <stdio.h>
#include <math.h>

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>

#include <KinectInput/KinectInput.h>
#include <KinectInput/KinectReproTools.h>

#include <Communication/OSC/OSCData.h>
#include <GeoPrimitives/Line.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/FBO.h>
#include <GLUtils/Spline2D.h>
#include <OpenCvUtils/CvKalmanFilter.h>
#include <Shaders/ShaderCollector.h>
#include <SceneNode.h>
#include <TrackObj.h>

#define STRINGIFY(A) #A

namespace tav
{    
    class SNKinectPainting2 : public SceneNode
    {
    public:
        enum drawMode { DIRECT_HAND=0, NI_HAND=1 };
        
        SNKinectPainting2(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNKinectPainting2();
        
        void initKinDependent();
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void procBlobs(double time, double dt);

        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);
        void onMouseButton(GLFWwindow* window, int button, int action, int mods);

        void getNiteHands(double time);
        void procBrushStrokes(double _time);
        void procSplash(double time);
        
        void applyBlur();
        void applyHeightMap(double time);
        void blendCanvasToAlpha();
        void blendBlurToAlpha();
        void clearTextures();
        void drawBrushStroke(float posX, float posY, float scale, float alphaMult,
                             GLuint brushTexId, GLuint hmTexId);
        void genBrushTex();
        void genBrush(float _x, float _y, glm::vec2 _brushTexOffs, float _scale, float _patternAmt,
                      float _pressure, GLuint brushTexId, GLuint heightMProfTex);
        void genSplash(double time);

        void showKinectInput();
        void setupBlendShader();
        void setupKinectDebugShader();
        
    private:
        KinectInput*            kin;
        GWindowManager*         winMan;
        OSCData*                osc;
        ShaderCollector*   		shCol;

        bool                    doDraw;
        bool                    doSplash;
        bool                    inited = false;
        bool                    requestClear = false;
        bool                    useMouse;
        bool                    debug = false;
        bool                    showKinectIn = true;
        bool                    hFlip = false;
                
        VAO*                    pointsVao;
        VAO*                    skeletonVao;
        
        FBO*                    drawFbo;

        FBO*                    applyHMFbo;
        FBO*                    canvasHeightMap;
        FBO*                    heightMap;
        FBO*                    pen;
        FBO*                    penPressProfile;
        FBO*                    perlinNoise;
        FBO*                    xBlendFboH;
        FBO*                    xBlendFboV;

        PingPongFbo*            blurFbo;
        PingPongFbo*            fastBlurPP;
        PingPongFbo*            canvasColor;
        PingPongFbo*            splash;
     
        Quad*                   debugQuad;
        Quad*                   quad;
        Quad*                   whiteQuad;
        
        Shaders*                blendShader;
        Shaders*                kinDebugShader;
        
        Shaders*                applyHeightMapShader;
        Shaders*                blurShader;
        Shaders*                colShader;
        Shaders*                splashShader;
        Shaders*                drawBrushColor;
        Shaders*                drawBrushCover;
        Shaders*                genBrushShader;
        Shaders*                genBrushHMShader;
        Shaders*                heightMapShader;
        Shaders*                noiseShader;
        Shaders*                texShader;
        Shaders*                texToAlpha;
        Shaders*                xBlendShaderH;
        Shaders*                xBlendShaderV;
        Shaders*                fastBlurHShader;
        Shaders*                fastBlurVShader;
        
        KinectReproTools*       kinRepro;
        Spline2D                intp;

        cv::Ptr<cv::SimpleBlobDetector> detector;
        std::vector<TrackObj>           trackObjs;

        
        glm::vec2               startBrushTexOffset;
        glm::vec2               oldM;
        glm::vec2               oldM2;
        glm::vec3               leftHand;
        glm::vec3               rightHand;
        glm::vec4               paintCol;
        
        float                   mouseX = 0;
        float                   mouseY = 0;
        float                   mouseZ = 0;
        float                   lastMouseZ = 0;

        float                   blendBlurAcc;
        float                   blendCanvAcc;
        float                   brushHardness;
        float                   brushHMAcc = 0.f;
        float                   brushSize;
        float                   canvasFdbk;
        float                   hStrength = 1.f;
        float                   hMStrength;
        float                   gravitation;
        float                   minZ;
        float                   moveThres;
        float                   paperAbsorbAmt;
        float                   paperTextStrength;
        float                   penDrawDist;
        float                   penChangeSpeed;
        float                   pressure;
        float                   scaleBrushTex;
        float                   smearing;
        float                   splashSize;
        float                   splashThres;
        float                   zDrawThres;
        
        float*                  coef;
        float*                  blurOffsV;
        float*                  blurOffsH;
        float**                 skeletonColors;

        double                  lastDrawTime = 0.0;
        double                  actTime;
        double                  preTime;
        double                  printFpsInt;
        double                  splashTimeThres;
        
        int                     brushTexSize;
        int                     clearWhite =0;
        int                     colFrameNr = -1;
        int                     colorCount;
        int                     depthFrameNr = -1;
        int                     drawPointPtr;
        int                     nrIntpSegs;
        int                     splashTexSize;
        int                     skelOffPtr;
        int                     debugIntrpSeg;
        int                     debugLineNrPoints=0;
        int                     noiseTexSize;
        
        int                     frameNr;
        int                     kinDeviceNr;

        const int               maxUsers;

        
        std::string             calibFileName;
        std::string             vs;
        std::string             fs;
        glm::mat4               penMatr;
        glm::mat4               heightMatr;
        glm::mat4               toTexMatr;
        glm::mat4               debugMatr;
        
        std::vector<glm::vec2>  lastMove;
        glm::vec2               lastPenDrawPos;
        int                     drawBuf = 0;
        
        double                  lastTime = -1.0;
        
        TextureManager*         brushProfileTex;
        TextureManager*         hmProfileTex;

        TextureManager*         kinDepth;
        TextureManager*         kinDepthDebug;
        
        KinectReproTools::mode  actMode;
        drawMode                actDrawMode;
        
        cv::Mat                 checkWarp;
        cv::Mat                 im_with_keypoints;
    };
}

#endif /* defined(__Tav_App__SNGLSLFluidTest__) */
