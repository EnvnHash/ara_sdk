//
// SNTFluidDepth.h
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

#include <GLUtils/GLSL/FastBlurMem.h>
#include <GLUtils/GLSL/FastBlur.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include <GLUtils/GLSL/GLSLParticleSystemFbo.h>
#include <GLUtils/GWindowManager.h>
#include <KinectInput/KinectInput.h>
#include <KinectInput/KinectReproTools.h>
#include <FPSTimer.h>
#include <SceneNode.h>

namespace tav
{
    class SNTFluidDepth : public SceneNode
    {
    public:
        enum drawMode { RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, OPT_FLOW, OPT_FLOW_BLUR, FLUID_VEL, DRAW };

        SNTFluidDepth(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTFluidDepth();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void mouseTest(double time);
        void startCaptureFrame(int renderMode, double time);
        virtual void captureFrame(int renderMode, double time);
        void initShaders();
        void initWhiteShdr();
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);

    private:
        FPSTimer						timer;

        OSCData*                        osc;

        KinectInput*                    kin;
        KinectReproTools* 				kinRepro;

        FastBlurMem*                    optFlowBlur;
        FastBlurMem*                    fblur;
        FastBlurMem*                    fblur2nd;
        GLSLFluid*                      fluidSim;
        GLSLOpticalFlow*                optFlow;

        GWindowManager*                 winMan;

        GLSLParticleSystemFbo*          ps;
        GLSLParticleSystemFbo::EmitData data;

        ShaderCollector*				shCol;

        Shaders*                        blendTexShader;
        Shaders*                        blurThres;
        Shaders*                        colShader;
        Shaders*                        depthThres;
        Shaders*                        edgeDetect;
        Shaders*                        mappingShaderTex;
        Shaders*                        mappingShaderPoints;
        Shaders*                        noiseShader;
        Shaders*                        subtrShadr;
        Shaders*                        texShader;
        Shaders*                       	whiteShdr;
        Shaders*                       	texShaderAlpha;

        FBO*                            threshFbo;
        FBO*                            particleFbo;

        PingPongFbo*                    edgePP;
        PingPongFbo*                    subtrPP;
        PingPongFbo*                    postBlurThreshPP;

        uint8_t*                        userMapConv;
        uint8_t*                        userMapRGBA;

        Quad*                           rawQuad;
        Quad*                           rotateQuad;
        Quad*							redQuad;

        bool                            inited;
        bool                            psInited;
        bool                            inactivityEmit = false;
        bool                            useAccu=false;
        bool                            saveRef=false;

        int                             nrParticle;
        int                             emitPartPerUpdate;
        int                             frameNr = -1;
        int                             colFrameNr = -1;
        int                             kinColorImgPtr = 0;
        int                             thinDownSampleFact;
        int                             noiseTexSize;
        int                             savedNr = 0;
        int                             fblurSize;
        int                             emitTexCounter;

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


    	float                           fastBlurAlpha = 0.4f;
    	float                           optFlowBlurAlpha = 0.76f;
    	float                           fluidColorSpeed = 1.f;
    	float                           fluidColTexForce = 0.325f;
    	float                           fluidDissip = 0.8f;
    	float                           fluidVelTexThres = 0.8f;
    	float                           fluidVelTexRadius = 0.08f;
    	float                           fluidVelTexForce = 0.74f;
    	float                           fluidSmoke = 0.457f;
    	float                           fluidSmokeWeight = 0.03f;
    	float                           fluidVelDissip = 0.93f;
    	float                           fluidTimeStep = 0.2f;
    	float                           veloBlend = 0.05f;
    	float                           depthAmp = 4.f;
    	float                           alpha = 0.f;


        glm::vec2                       oldM;
        glm::vec2                       forceScale;

        glm::vec4*                      fluidAddCol;
        glm::vec4*                      partEmitCol;

        drawMode                        actDrawMode;

        boost::thread*                  capture_Thread = 0;
    };
}
