//
// SNGam_Danza.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <thread>

#include <lo/lo.h>


#include <GLUtils/GLSL/FastBlurMem.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <GLUtils/GLSL/GLSLParticleSystemCS.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include <GLUtils/GLSL/PixelCounter.h>
#include <GLUtils/PingPongFbo.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/NoiseTexNV.h>
#include <GLUtils/PropoImage.h>
#include <GLUtils/ThreshFbo.h>
#include <GLUtils/UniformBlock.h>
#include <nvidiaDecoder/NvDecodeGL.h>
#include <KinectInput/KinectInput.h>
#include <KinectInput/KinectReproTools.h>
#include <Shaders/ShaderCollector.h>
#include <AnimVal.h>
#include <Median.h>
#include <SceneNode.h>
//#include <VideoTextureCv.h>
//#include <VideoTextureThread.h>
#include <FFMpegDecode.h>
//#include <ImgSeqPlayer.h>


namespace tav
{

class SNGam_Danza : public SceneNode
{
public:
    enum vtLevels { VT_PRESENT, VT_SELECT, VT_DANCE0, VT_DANCE1, VT_DANCE2, VT_DANCE3, VT_NR_LEVELS };
    enum drawMode { PRESENT, SELECT, DANCE, NR_MODES };
    enum debugDrawMode { RAW_DEPTH, USER_MAP, TRANS_DEPTH, DEPTH_THRESH, DEPTH_BLUR, OPT_FLOW, OPT_FLOW_BLUR, FLUID, NONE };

	SNGam_Danza(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
    ~SNGam_Danza();

    void initParticleSystem();
    void initTextures();
    void initOscPar();
    void initSelectionIcons();
    void initFluidSim();

    void initUserMapShader();
    void initDrawLumaKey();
    void initDrawShdr();
    void initDrawSilBlackShdr();
    void initSilBackShdr();
    std::string getUpdtShader();

    void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
    void drawDebug(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
    void drawSelect(camPar* cp);
    void drawDance(double time, double dt, camPar* cp);
    void drawParticles(camPar* cp, double time, double dt);

    void cleanUp();
    void update(double time, double dt);

    void updateParticles(double time, double dt);
    void emitParticles(double time, double dt);

    void onKey(int key, int scancode, int action, int mods);

    void switchTo(drawMode _mode, double time);
    void songEndCb();

    void saveCalib();
    void loadCalib();

private:

	lo_address 					scAddr;

    FastBlurMem*            	fblur;
    FastBlurMem*            	fblur2nd;
    FastBlurMem*            	optFlowBlur;
    FBO*						sil_back_fbo;
	GLSLFluid*                  fluidSim;
    GLSLOpticalFlow*			optFlow;
    GLSLParticleSystemCS* 		mParticles;
	GWindowManager*				winMan;
	KinectInput*				kin;
    KinectReproTools*			kinRepro;
	NoiseTexNV*					noiseTex;

	PixelCounter*				totPixCounter;
	PixelCounter**				pixCounter;
    PingPongFbo*				bailarina_fbo;
    Quad*                   	rawQuad;
    Quad*                   	rotateQuad;
	ShaderParams 				mShaderParams;
	ShaderCollector*			shCol;

	Shaders* 					drawSilBlack;
	Shaders*					useMapShader;
	Shaders* 					mRenderProg;
	Shaders* 					luma_key_shader;
	Shaders*					sil_back_shader;
	Shaders* 					texShader;
	Shaders* 					texAlphaShader;
	Shaders* 					yuvShader;

    TextureManager*				flowers;
    TextureManager*				flowers_normals;
    TextureManager				flowers_back;
    TextureManager*				flowers_fill;
    TextureManager				text_select;
    TextureManager*				select_icons;
    TextureManager*				dance_back_texs;
    TextureManager				userMapTex;

    ThreshFbo*					threshFbo;
	VAO*						testVAO;

    GLuint 						mUBO;
    GLuint						flowers_3Dtex;

    GLint  						uboSize;
    GLuint 						uboIndex;

    GLint*						transTexId=0;

    bool						inited=false;
    bool						enableInteract;
    bool						switchToSelectFromDance;
    bool						blockIcons;
    bool						requestSwitchToPresent;
    bool						debugLoop;
    bool						checkForSongEnd=false;

    int                     	frameNr;

    unsigned int               	actSong;
    unsigned int               	fblurSize;
    unsigned int              	flSize;
    unsigned int				nrSongs;
    unsigned int				songPtr;
    unsigned int				rescaleIcon=-1;
    unsigned int				vidIt=0;
    unsigned int				nrVidThreads;
    unsigned int				actIt=0;

    float						alpha;
    float						fastBlurAlpha;
	float						spriteSize=0.006f;

	float						kinRotX;
	float						kinRotY;
	float						kinRotZ;
	float						kinTransX;
	float						kinTransY;
	float						kinTransZ;
	float						screenScaleX;
	float						screenScaleY;
	float						screenTransX;
	float						screenTransY;
	float						pointSize;
	float						pointWeight;
	float						nearThres;
	float						farThres;
	float						lumaThres;
	float						lumaMult;
	float						optFlowVelAmt;
	float						optFlowPosAmt;
	float 						fluidVelTexForce;
	float 						partSpeed;

	float						canvasRotAngle;
	float 						opacity;
	float						selectIconBorder;
	float 						selectIconRelSize;
	float 						selectIconThres;

	float						silColDepth;
	float						windAmt;
	float 						nrPartPerEmit;


	float 						soundLowLevel;
	float 						soundHighLevel;

	Median<float>*				totActivity;

    double              		lastTime = 0.0;
    double						lastPartUpdt = 0.0;
    double						lastEmit = 0.0;
    double						fadeOutTime;
    double						actUpdtInt;
    double						lastActUpdtTime=0.0;
    double						totActSwitchLevel;
    double						timeToSwitchDanceIdle;
    double						requestSwitchToPresentTime;


    double						lastDebugSwitchTime;
    double						timeToSwitch = 4.0;
    double						timeToStopSong=0.0;

    double 						changeVidTime;
    double 						lastChangeVidTime =0.0;

    glm::ivec2					back_vid_size;

	glm::mat4 					view;
    glm::mat4 					proj;
    glm::vec4* 					initPars;
    glm::mat4 					transMat;
    glm::mat4 					transMat2D;

    glm::vec4* 					silRgbFact;

    glm::mat4*					icon_mat;

    drawMode                	actDrawMode;
    debugDrawMode				debugDrawMode;
    drawMode					requestSwitch;

    std::string					calibFileName;
	std::vector<std::string>	songFilesAudio;

	std::vector<std::string>	vt_back_paths;
	std::vector<std::string>	vt_dance_paths;

	FFMpegDecode*				vt_back=0;
	FFMpegDecode*				vt_dance=0;

	//ImgSeqPlayer*				vt_back_img_seq;


	AnimVal<float>*				blendVal0;
	AnimVal<float>*				blendVal1;
	AnimVal<float>**			icon_scale_offs;

	std::vector<double>			songFilesAudioDur;
	vtLevels					actVtLevel;

};

}
