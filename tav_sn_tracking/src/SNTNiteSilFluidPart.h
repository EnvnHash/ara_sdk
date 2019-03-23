//
// SNTNiteSilFluidPart.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <math.h>
#include <boost/thread.hpp>

#include <headers/gl_header.h>
#include <GLUtils/GLSL/FastBlurMem.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include <GLUtils/GLSL/GLSLParticleSystemFbo.h>
#include <GLUtils/GLSL/GLSLParticleSystem2.h>
#include <KinectInput/KinectInput.h>
#include <SceneNode.h>

namespace tav
{
    class SNTNiteSilFluidPart : public SceneNode
    {
    public:
                
        SNTNiteSilFluidPart(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTNiteSilFluidPart();
        
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void updateSil(double dt);
        void mouseTest(double time);
        void procUserMaps(double dt);
        void genWindTex();
        void renderPartFbo();
        void startCaptureFrame(int renderMode, double time);
        virtual void captureFrame(int renderMode, double time);
        void set3CamViewPorts();
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);
        
    private:
        OSCData*                        osc;
        KinectInput*                    kin;
        FastBlurMem*                    fblur;
        GLSLFluid*                      fluidSim;
        GLSLOpticalFlow*                optFlow;
        ShaderCollector*				shCol;

        GLSLParticleSystemFbo*          ps;
        GLSLParticleSystemFbo::EmitData data;

//        GLSLParticleSystem2*          ps;
//        GLSLParticleSystem2::EmitData    data;
        
        Shaders*                        mappingShaderTex;
        Shaders*                        mappingShaderPoints;
        
        Shaders*                        colShader;
        Shaders*                        texShader;
        Shaders*                        blendTexShader;
        Shaders*                        noiseShader;
        Shaders*                        edgeDetect;
        Shaders*                        xBlendShaderH;
        Shaders*                        xBlendShaderV;

        FBO*                            perlinNoise;
        FBO*                            xBlendFboH;
        FBO*                            xBlendFboV;

        PingPongFbo**                   renderPartPP;
        PingPongFbo*                    edgePP;
        TextureManager*                 logoTex;
        TextureManager*                 userMapTex;
        TextureManager**                kinColorTex;
        TextureManager*                 tempDepthTex;

        NISkeleton*                     nis;

        uint8_t*                        userMapConv;
        uint8_t*                        userMapRGBA;
        
        Quad*                           rawQuad;
        Quad*                           rotateQuad;
        Quad*                           logoQuadFullScreen;
        Quad*                           logoQuadCenterScreen;
        
        bool                            inited=false;
        bool                            psInited;
        bool                            inactivityEmit = false;

        int                             nrParticle;
        int                             emitPartPerUpdate;
        int                             frameNr = -1;
        int                             colFrameNr = -1;
        int                             kinColorImgPtr = 0;
        int                             nrForceJoints;
        int                             thinDownSampleFact;
        int                             noiseTexSize;
        int                             renderMode;
        int                             nrCameras;
        int                             savedNr = 0;
        int                             fblurSize;
        int                             emitTexCounter;

        unsigned int                    nrTestPart;
        const int                       maxNrUsers = 4;
        
        double                          mouseX = 0;
        double                          mouseY = 0;
        double                          captureIntv;
        double                          lastCaptureTime = 0;
        double                          niteResetIntv;
        double                          lastNiteReset = 0;
        double                          inActCounter = 0;
        double                          inActNiResetInt = 0;
        double                          inActEmitTime = 0;
        double                          inActAddNoiseInt = 0;
        
        float                           flWidth;
        float                           flHeight;
        float                           partTexScale;
        float                           screenYOffset;
        float                           depthTexMedian;
        
        glm::vec2                       oldM;
        glm::vec2                       forceScale;
        glm::mat4                       identMatr;
        
        glm::mat4*                      pointModMatrices;
        glm::mat4*                      texModMatrices;
        glm::mat4                       projMatrices;
        glm::mat4*                      multCamMatr;

        glm::vec2                       leftScreenDim;
        glm::vec2                       centerScreenDim;
        glm::vec2                       rightScreenDim;
        glm::vec2                       totScreenDim;
        glm::vec4*                      vp;
        
        glm::vec4*                      fluidAddCol;
        glm::vec4*                      partEmitCol;

        GLMCamera*                      stdCam;
        GLMCamera**                     multCam;
        
        cv::Mat                         capturePic1Cam;
        cv::Mat                         fCapturePic1Cam;
        cv::Mat                         capturePic3Cam;
        cv::Mat                         fCapturePic3Cam;
        
        boost::thread*                  capture_Thread = 0;
    };
}
