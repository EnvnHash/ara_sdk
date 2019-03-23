//
// SNTrustHeightMap.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <math.h>
#include <stdint.h>
#include <boost/thread.hpp>

#include <GeoPrimitives/QuadArray.h>
#include <GLUtils/GLSL/FastBlurMem.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include <GLUtils/GLSL/GLSLHistogram.h>
#include <GLUtils/GWindowManager.h>
#include <KinectInput/KinectInput.h>
#include <VideoTextureCvActRange.h>
#include <SceneNode.h>

namespace tav
{
    class SNTrustHeightMap : public SceneNode
    {
    public:
        enum drawMode { RAW_DEPTH, OPT_FLOW, OPT_FLOW_BLUR, FLUID_VEL, FLUID_AND_SHAPE, DRAW };

        SNTrustHeightMap(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTrustHeightMap();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void initFluidHeightShader();
        void initAddShapeShader();
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);

    private:
        VideoTextureCvActRange*	        vt;

        FastBlurMem*                    optFlowBlur;

        GLSLFluid*                      fluidSim;
        GLSLOpticalFlow*                optFlow;

        GWindowManager*                 winMan;

        ShaderCollector*				shCol;

        Shaders*                        colShader;
        Shaders*                        texShader;
        Shaders*                        posTexShdr;
        Shaders*                        normShader;
        Shaders*                        fluidHeightShdr;
        Shaders*                        addShapeShdr;

        FBO*                            zoomFbo;
        FBO*                            normFbo;
        FBO*                            fluidAndShape;
        TextureManager*                 litsphereTex;
        TextureManager*                 bumpMap;

        uint8_t*                        userMapConv;
        uint8_t*                        userMapRGBA;

        QuadArray*                      hmGrid;
        Quad*                           rawQuad;
        Quad*                           rotateQuad;

        bool                            inited;
        bool                            psInited;
        bool                            inactivityEmit = false;
        bool                            useAccu=false;
        bool                            saveRef=false;

        int                             frameNr = -1;
        int                             colFrameNr = -1;
        int                             kinColorImgPtr = 0;
        int                             savedNr = 0;
        int                             fblurSize;
        int                             actUplTexId = 0;
        int								lastTexId = -1;

        unsigned int                    nrTestPart;
        const int                       maxNrUsers = 4;

        double                          mouseX = 0;
        double                          mouseY = 0;
        double                          captureIntv;
        double                          lastCaptureTime = 0;
        double                          inActCounter = 0;
        double                          inActEmitTime = 0;
        double                          inActAddNoiseInt = 0;
        double                          lastTime = 0.0;

        float                           flWidth;
        float                           flHeight;
        float                           partTexScale;
        float                           screenYOffset;

        float				alpha=1744.f;
        float				depthTresh=1744.f;
        float				fastBlurAlpha=0.4f;
        float				optFlowBlurBright=0.76f;
        float				optFlowBlurAlpha=0.76f;
        float				heightOffs=-5.8f;
        float				heightScale=1.f;
        float				fluidColTexForce=0.1f;
        float				timeStep=0.1f;
        float				fluidVelTexThres=0.8f;
        float				fluidVelTexRadius=0.08f;
        float				fluidVelTexForce=0.54f;
        float				fluidSmoke=0.457f;
        float				fluidSmokeWeight=0.03f;
        float				fluidVelDissip=0.93f;
        float				fluidScaleXY=6.f;
        float				fluidNormHeightAdj=0.26f;
        float				fluidColMix=0.97f;
        float				shapeHeight=1.61f;
        float				shapeAddFluidHeight=0.15f;
        float				shapeHeightAlpha=0.1f;
        float				rotScene=0.f;

        glm::vec2                       oldM;
        glm::vec2                       forceScale;

        glm::vec4*                      fluidAddCol;
        glm::vec4*                      partEmitCol;

        drawMode                        actDrawMode;

        boost::thread*                  capture_Thread = 0;
    };
}
