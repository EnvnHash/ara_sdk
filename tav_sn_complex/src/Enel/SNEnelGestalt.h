//
// SNEnelGestalt.h
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

#include <headers/opencv_headers.h>

#include <GLUtils/GLSL/FastBlurMem.h>
#include <GLUtils/GLSL/FastBlur.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <GLUtils/GLSL/GLSLParticleSystemFbo.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include <GLUtils/GLSL/GodRays.h>
#include <GLUtils/GLSL/GLSLHistogram.h>

#include <GLUtils/Typo/NVTextBlock.h>
#include <GLUtils/Spline2D.h>
#include <Shaders/ShaderBuffer.h>

#include <SceneNode.h>
#include <KinectInput/KinectInput.h>
#include <GLUtils/GWindowManager.h>
#include <GeoPrimitives/QuadArray.h>
#include <FPSTimer.h>

#include <OpenCvUtils/CvKalmanFilter.h>


namespace tav
{
    class SNEnelGestalt : public SceneNode
    {
    public:
        enum drawMode { RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, OPT_FLOW, OPT_FLOW_BLUR, FLUID_VEL, SENTENCES, DRAW };

        typedef struct {
        	ShaderBuffer<glm::vec4>* buf=0;
        	glm::vec4* cpuBuf =0;
        	glm::vec4 center;
        	double startTime = 0.0;
        	double lifeTime = 0.0;
            double radius = 0.0;
            double area = 0.0;
            double circularity=0.0;
            double confidence = 0.0; // etwa komplexitaet
            double convexity = 0.0;
            int nrRawPoints = 0;
        } contPar;
        
        struct CV_EXPORTS Center
        {
             cv::Point2d location;
             double radius;
             double area;
             double circularity;
             double confidence;
             double convexity;
        };

        struct CV_EXPORTS_W_SIMPLE BlobParams
        {
            CV_WRAP BlobParams() : thresholdStep(10), minThreshold(50), maxThreshold(220),
            minRepeatability(2), minDistBetweenBlobs(10), filterByColor(false), blobColor(0),
			filterByArea(true), minArea(100), maxArea(100000), filterByCircularity(false),
			minCircularity(0.8f), maxCircularity(std::numeric_limits<float>::max()),
			filterByInertia(false), minInertiaRatio(0.1f), maxInertiaRatio(std::numeric_limits<float>::max()),
			filterByConvexity(false), minConvexity(0.95f), maxConvexity(std::numeric_limits<float>::max())
            {}

            CV_PROP_RW float thresholdStep;
            CV_PROP_RW float minThreshold;
            CV_PROP_RW float maxThreshold;
            CV_PROP_RW size_t minRepeatability;
            CV_PROP_RW float minDistBetweenBlobs;

            CV_PROP_RW bool filterByColor;
            CV_PROP_RW uchar blobColor;

            CV_PROP_RW bool filterByArea;
            CV_PROP_RW float minArea, maxArea;

            CV_PROP_RW bool filterByCircularity;
            CV_PROP_RW float minCircularity, maxCircularity;

            CV_PROP_RW bool filterByInertia;
            CV_PROP_RW float minInertiaRatio, maxInertiaRatio;

            CV_PROP_RW bool filterByConvexity;
            CV_PROP_RW float minConvexity, maxConvexity;
        };


        SNEnelGestalt(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNEnelGestalt();
        
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void procProgress(double time, double dt);
        void renderTypo(double time, camPar* cp);
        void renderTypoVelTex(double time, camPar* cp);
        void renderSilhouettes(double time);

        void initShaders();
        void initSentShdr();
        void initGradShader();
        void initLineShader();
        void initLineTriShader();
        void initAccumShdr();
        void initDiffShdr();

        void update(double time, double dt);
        void procContours(GLuint texId, double time);
        void trackBlobs();

        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);
        
    private:
        BlobParams 						params;

        FPSTimer						timer;

        KinectInput*                    kin;

        FastBlurMem*                    optFlowBlur;
        FastBlurMem*                    fblur;
        FastBlurMem*                    fblur2nd;
        FastBlurMem*					lineBlur;

   //     FreetypeTex**					ft;

		GLSLFluid*                      fluidSim;
        GLSLOpticalFlow*                optFlow;
    	GLSLHistogram*					histo;
        GodRays*						godRays;

        GWindowManager*                 winMan;
        
        GLSLParticleSystemFbo*          ps;
        GLSLParticleSystemFbo::EmitData data;

        NVTextBlock**					textBlocks;

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
        Shaders*                        fluidLineShdr;
        Shaders*                        gradShader;
        Shaders*						texAlphaShader;
        Shaders*						sentenceShader;
        Shaders*                        lineShader;
        Shaders*                        accumShader;
        Shaders*						stdMultiTex;
        Shaders*						diffShader;
        Shaders*						stdColAlpha;

        TextureManager*					enelLogo;
        TextureManager*					muevate;

        FBO*                            accumFbo;
        FBO*                            diffFbo;
        FBO*                            gradFbo;
        FBO*							lineFbo;
        FBO*							lineSnapFbo;
        FBO*                            particleFbo;
       // FBO*							typoFbo;
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

        VAO*							rawPointVAO;
        
        bool                            inited;
        bool                            psInited;
        bool                            inactivityEmit = false;
        bool                            useAccu=false;
        bool                            saveRef=false;
        bool							textInited=false;
        bool							startSwitch = false;
        bool							block = false;
        bool							fluidMode=false;
        bool							drawParticles =false;
        bool							reqSnapShot=false;
        bool							reqDeleteSnapBuf = false;

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
        unsigned int                    nrSilhouttePoints;
        unsigned int					nrSentences;
        unsigned int					sentPtr=0;
        unsigned int					nrEnelColors;
        unsigned int					enelColPtr=0;

        const int                       maxNrUsers = 4;
        
        double                          mouseX = 0;
        double                          mouseY = 0;
        double                          captureIntv;
        double                          lastCaptureTime = 0;
        double                          inActCounter = 0;
        double                          inActEmitTime = 0;
        double                          inActAddNoiseInt = 0;
        double                          lastTime = 0.0;
        double							lastEmitContourTime = 0.0;
        double							emitContourInt = 1.0;
        double							startSwitchTime = 0;
        double							switchFade = 0;
        double							switchProg=0;
        double							stopSwitchTime=0;

        double							fadeDown = 0;

        float                           flWidth;
        float                           flHeight;
        float                           partTexScale;
        float                           screenYOffset;
        float							typoProgress;
        
        float depthThresh;
        float fastBlurAlpha;
        float optFlowBlurAlpha;
        float contThresh;
        float lineFdbk;

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

        float silLifeTime;
        float lineWidth;
        float impFreq;
        float impFreq2;
        float nrSilInstances;
        float instScaleAmt;
        float silMed;
        float timeOffsScale;
        float timeScaleZoom;
        float rotGradOffs;
        float lineBlurAlpha;
        float lineBlurOffs;
        float sentFluidDist;
        float typoSpeed;
        float typoMoveAmt;
        float accumAlpha;
        float godRaySpeed;

    	float							trig;
    	float							clear;

        float				exp=0.00235f;
        float				dens=0.47f;
        float				decay=0.9999f;
        float				weight=5.53f;
        float				lightX=0.5f;
        float				lightY=0.5f;
        float				grAlpha=1.f;
        std::string*		words;
        glm::vec2                       oldM;
        glm::vec2                       forceScale;
        
        glm::vec4*                      fluidAddCol;
        glm::vec4*                      partEmitCol;
        glm::vec4*                      enel_colors;
        
        glm::vec4						colBase;
        glm::vec4						colPeak;
        glm::vec4						eCol;

        glm::mat4						typoPosMat;

        drawMode                        actDrawMode;
        
        cv::Mat							img;
    	cv::Mat 						src_gray;

        cv::Point2f						imgDim;
        std::vector<cv::Rect> 			boundRect;

        std::vector< std::vector<cv::Point> > contours;
        std::vector< std::vector<contPar> >	silhoutte_frames;
        std::vector<contPar>			render_silhouttes;
        std::vector< std::vector<CvKalmanFilter*> > kalmanFilters;

    };
}
