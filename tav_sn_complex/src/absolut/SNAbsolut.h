//
// SNAbsolut.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>

#include <KinectInput/KinectInput.h>
#include <Communication/OSC/OSCData.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <GLUtils/GLSL/GLSLParticleSystem2.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include <GLUtils/GLSL/FastBlurMem.h>
#include <GLUtils/GLSL/GLSLHistogram.h>

#include <GLUtils/FBO.h>
#include <GLUtils/GWindowManager.h>
#include <SceneNode.h>

#include <Median.h>


namespace tav
{
    class SNAbsolut : public SceneNode
    {
    public:
        enum drawMode { RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, OPT_FLOW, OPT_FLOW_BLUR, OPT_FLOW_BLUR_BW, FLUID_VEL, DRAW };

        SNAbsolut(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNAbsolut();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void updateFade(double time);
        void initShaders();

        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);
        
    private:
        OSCData*                        osc;
        
        GLSLFluid*                      fluidSim;
        GLSLParticleSystem2*            ps;
        GLSLParticleSystem2::EmitData   data;
        GLSLOpticalFlow*                optFlow;
        GLSLHistogram*					histo;

        FBO*                            threshFbo;
        FBO*                            bwActFbo;

        FastBlurMem*                    optFlowBlur;
        FastBlurMem*                    fblur;
        FastBlurMem*                    fblur2nd;

        KinectInput*                    kin;

        ShaderCollector*    			shCol;

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
        Shaders*                        texAlphaShader;
        Shaders*                        optFlow2BwShdr;


        Quad*                           quad;
        Quad*                           rawQuad;
        Quad*                           rotateQuad;

        TextureManager**                texs;
        TextureManager**                frase_solid_tex;
        TextureManager**                frase_glow_tex;

        TextureManager*					backTex;
        TextureManager*					frontTex;
        TextureManager*					redDotTex;
        TextureManager*					absolut_logo_tex;

        GWindowManager*					winMan;

        bool                            inited;
        bool                            psInited;
        bool							gotActivity = false;
        bool							requestOff = false;
        bool							blockChange = false;

        int                             nrPartLeaves;
        int                             maxNrPart;
        int                             nrForceJoints;
        int                             frameNr;
        int                             kinTexPtr=0;

        unsigned int					fblurSize;
        unsigned int					numLeaveTexs;
        unsigned int					numFrases;
        unsigned int					fraseBlendFrom=0;
        unsigned int					fraseBlendTo=0;

        double                          mouseX = 0;
        double                          mouseY = 0;
        double							initActivityTime=0;
        double							destinyTime = 10.0;
        double							progress=0;
        double 							offTime=0;
        double							breakTime;
        double							blockStart=0;
        double							timeToSetOff=0;
        double							wasSetOffOn=0;
        
        float							fadeOutFact = 0.1f;
        float							frasesMix=0.f;
        Median<float>*					fraseBlendFromAlphaMed;
        Median<float>*					fraseBlendToAlphaMed;


        float							fraseBlendFromAlpha=0.f;
        float							fraseBlendToAlpha=0.f;

        float                           flWidth;
        float                           flHeight;
        
        float                           scaleFact;
        float                           transX;
        float                           transY;
        float							bottlePos;
        float							logoDist;
        

        float depthThresh;
        float fastBlurAlpha;
        float optFlowBlurAlpha;
        float fluidColorSpeed;
        float fluidColTexForce;
        float fluidDissip;
        float fluidVelTexThres;
        float fluidVelTexRadius;
        float fluidVelTexForce;
        float fluidSmoke;
        float fluidSmokeWeight;
        float fluidVelDissip;
        float partColorSpeed;
        float partFdbk;
        float partFriction;
        float partAlpha;
        float partBright;
        float partVeloBright;
        float veloBlend;
        float reposFact;
        float zPos;
        float activityThres;

        GLint*							leaveTexUnits;
        
        glm::vec2                       oldM;
        glm::vec2                       forceScale;

        glm::vec3                       kCamPos;
        glm::vec3                       camPos;
        
        std::vector<std::string>        args;

        drawMode                        actDrawMode;

        glm::mat4						frase_from_mat;
        glm::mat4						frase_to_mat;

        float* 							frases_alpha;

    };
}
