//
// SNTSilhouetteRGB.h
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
    class SNTSilhouetteRGB : public SceneNode
    {
    public:
        enum drawMode { RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, DRAW };
        
        SNTSilhouetteRGB(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTSilhouetteRGB();
        
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void startCaptureFrame(int renderMode, double time);
        virtual void captureFrame(int renderMode, double time);
        void initDepthThresShader();
        void initRGBOffsetShader();
    
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);
        
    private:
        OSCData*                        osc;
        KinectInput*                    kin;
        
        FastBlurMem*                    fblur;
        FastBlurMem*                    fblur2nd;
        FastBlurMem*                    histoBlur;
        
        GLSLHistogram*                  histo;

        GWindowManager*                 winMan;

        Shaders*                        colShader;
        Shaders*                        depthThres;
        Shaders*                        texShader;
        Shaders*                        rgbOffsetShdr;
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
        float                           shapeHeight= 1.61f;
        float                           shapeAddFluidHeight= 0.15f;
        float                           shapeHeightAlpha= 0.1f;
        float                           rotScene= 0.f;


        drawMode                        actDrawMode;
        
        boost::thread*                  capture_Thread = 0;
    };
}
