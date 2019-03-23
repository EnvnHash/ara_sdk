//
// SNEnelFluid.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <math.h>
#include <stdint.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/string_cast.hpp>

#include <GLUtils/GLSL/FastBlurMem.h>
#include <GLUtils/GLSL/FastBlur.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <GLUtils/GLSL/GLSLParticleSystemFbo.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include <GLUtils/GLSL/GLSLHistogram.h>

#include <GLUtils/Typo/NVTextBlock.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/Spline2D.h>

#include <Shaders/ShaderBuffer.h>

#include <SceneNode.h>
#include <KinectInput/KinectInput.h>
#include <GeoPrimitives/QuadArray.h>


namespace tav
{
    class SNEnelFluid : public SceneNode
    {
    public:
        enum drawMode { RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, OPT_FLOW, OPT_FLOW_BLUR, FLUID_VEL, SENTENCES, DRAW };

        SNEnelFluid(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNEnelFluid();
        
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void procActivity();
        void renderTypo(double time, camPar* cp);
        void renderTypoVelTex(double time, camPar* cp);

        void initShaders();
        void initSentShdr();
        void initAccumShdr();
        void initDiffShdr();

        void update(double time, double dt);

        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);
        
    private:

        KinectInput*                    kin;

        FastBlurMem*                    optFlowBlur;
        FastBlurMem*                    fblur;
        FastBlurMem*                    fblur2nd;

       // FreetypeTex**					ft;

		GLSLFluid*                      fluidSim;
        GLSLOpticalFlow*                optFlow;
    	GLSLHistogram*					histo;

        GWindowManager*                 winMan;
        
        GLSLParticleSystemFbo*          ps;
        GLSLParticleSystemFbo::EmitData data;

        std::vector< std::vector< std::string > > text_sets;
        NVTextBlock**  					text_block;

        ShaderCollector*				shCol;

        Shaders*                        blurThres;
        Shaders*                        colShader;
        Shaders*						diffShader;
        Shaders*                        depthThresShdr;
        Shaders*                        texShader;
        Shaders*						texAlphaShader;
        Shaders*						sentenceShader;
        Shaders*                        subtrShadr;
        Shaders*						stdMultiTex;

        FBO*                            accumFbo;
        FBO*                            diffFbo;
        FBO*                            particleFbo;
        FBO*							typoFbo;
        FBO*                            threshFbo;
        FBO*                            typoMaskFbo;


        PingPongFbo*                    edgePP;
        PingPongFbo*                    subtrPP;
        PingPongFbo*                    postBlurThreshPP;

        uint8_t*                        userMapConv;
        uint8_t*                        userMapRGBA;

        Quad*                           rawQuad;
        Quad*                           rotateQuad;
        Quad*                           colQuad;
        QuadArray*						quadAr;
        
        bool                            inited;
        bool                            psInited;
        bool                            useAccu=false;
        bool                            saveRef=false;
        bool							textInited=false;
        bool							startSwitch = false;
        bool							block = false;
        bool							drawParticles =false;
        bool							sentSwitch = false;

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
        unsigned int					nrSentences;
        unsigned int					nrBatchSentences;

        unsigned int					textSetPtr;
        unsigned int					sentPtr=0;
        unsigned int					nrEnelColors;
        unsigned int					enelColPtr=0;
        unsigned int					backColPtr=0;

        unsigned int					fadeFromBackCol;
        unsigned int					fadeToBackCol;

        const int                       maxNrUsers = 4;
        
        double                          mouseX = 0;
        double                          mouseY = 0;
        double                          lastTime = 0.0;
        float							colorStayTime = 3.0;
        float							lastChangeColTime = 0.0;
        float							backColFadeTime = 0.0;

        float							sentSwitchInt;
        float							lastSentSwitchTime = 0.0;
        float							sentSwitchTime = 0.0;

        float                           flWidth;
        float                           flHeight;
        float                           partTexScale;
        float                           screenYOffset;
        float							typoProgress;
        float							backBright;
        float							typoYOffs;
        
        float							oscTextSetNr=0;
        float							lastOscTextSetNr=0;

        float depthThresh;
        float fastBlurAlpha;
        float optFlowBlurAlpha;
        float contThresh;

        float fluidColorSpeed;
        float fluidColTexForce;
        float fluidDissip;
        float fluidVelTexThres;
        float fluidVelTexRadius;
        float fluidVelTexForce;
        float fluidSmoke;
        float fluidSmokeWeight;
        float fluidVelDissip;
        float fluidColAmp;

        float partColorSpeed;
        float partFdbk;
        float partFriction;
        float partAlpha;
        float partBright;
        float partVeloBright;
        float veloBlend;

        float sentFluidDist;
        float typoSpeed;
        float typoMoveAmt;
        float accumAlpha;
        float activityMax;
        float typoSize;


        float interactRate;

        GLint* 							typoTex;

        glm::vec2                       oldM;
        glm::vec2                       forceScale;
        
        glm::vec4*                      partEmitCol;
        glm::vec4*                      enel_colors;
        glm::vec4						eCol;
        glm::vec4						backCol;

        glm::mat4						typoPosMat;

        drawMode                        actDrawMode;
        
        cv::Mat							img;
    	cv::Mat 						src_gray;

        cv::Point2f						imgDim;
        std::vector<cv::Rect> 			boundRect;
        Median<float>*					activity;

    };
}
