//
// SNTFluidDepthHeightMap.h
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
#include <GLUtils/GLSL/GLSLHistogram.h>
#include <SceneNode.h>
#include <KinectInput/KinectInput.h>
#include <GLUtils/GWindowManager.h>
#include <GeoPrimitives/QuadArray.h>

namespace tav
{
    class SNTFluidDepthHeightMap : public SceneNode
    {
    public:
        enum drawMode { RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, OPT_FLOW, OPT_FLOW_BLUR, FLUID_VEL, DRAW };
        
        SNTFluidDepthHeightMap(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTFluidDepthHeightMap();
        
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void startCaptureFrame(int renderMode, double time);
        virtual void captureFrame(int renderMode, double time);
        void initDepthThresShader();
        void initFluidHeightShader();
        void initAddShapeShader();
    
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);
        
    private:
        OSCData*                        osc;
        KinectInput*                    kin;
        
        FastBlurMem*                    optFlowBlur;
        FastBlurMem*                    fblur;
        FastBlurMem*                    fblur2nd;
        FastBlurMem*                    histoBlur;
        
        GLSLFluid*                      fluidSim;
        GLSLOpticalFlow*                optFlow;
        GLSLHistogram*                  histo;

        GWindowManager*                 winMan;

        Shaders*                        colShader;
        Shaders*                        depthThres;
        Shaders*                        texShader;
        Shaders*                        posTexShdr;
        Shaders*                        normShader;
        Shaders*                        fluidHeightShdr;
        Shaders*                        addShapeShdr;
        ShaderCollector*				shCol;

        FBO*                            threshFbo;
        FBO*                            particleFbo;
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
        int                             thinDownSampleFact;
        int                             savedNr = 0;
        int                             fblurSize;

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
        
        float                           depthTresh = 1044.f;
        float                           fastBlurAlpha= 0.4f;
        float                           optFlowBlurAlpha= 0.76f;
        float                           fluidHeightOffs= -5.8f;
        float                           fluidColorSpeed= 0.3f;
        float                           fluidColTexForce= 0.05f;
        float                           fluidDissip= 0.8f;
        float                           fluidVelTexThres= 0.8f;
        float                           fluidVelTexRadius= 0.08f;
        float                           fluidVelTexForce= 0.54f;
        float                           fluidSmoke= 0.457f;
        float                           fluidSmokeWeight= 0.03f;
        float                           fluidVelDissip= 0.93f;
        float                           fluidScaleXY= 6.f;
        float                           fluidNormHeightAdj= 0.26f;
        float                           fluidColMix= 0.97f;
        float                           shapeHeight= 1.61f;
        float                           shapeAddFluidHeight= 0.15f;
        float                           shapeHeightAlpha= 0.1f;
        float                           rotScene= 0.f;



        glm::vec2                       oldM;
        glm::vec2                       forceScale;
        
        glm::vec4*                      fluidAddCol;
        glm::vec4*                      partEmitCol;
        
        drawMode                        actDrawMode;
        
        boost::thread*                  capture_Thread = 0;
    };
}
