/*
 * SNEnelPalabras.h
 *
 *  Created on: 14.10.2017
 *      Author: sven
 */

#ifndef SRC_ENEL_SNENELPALABRAS_H_
#define SRC_ENEL_SNENELPALABRAS_H_

#pragma once

#include <SceneNode.h>
#include <GLUtils/Typo/FreetypeTex.h>
#include <GeoPrimitives/Quad.h>
#include <KinectInput/KinectInput.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <GLUtils/GLSL/GLSLParticleSystemFbo.h>
#include <GLUtils/GLSL/GLSLHistogram.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include <GLUtils/GLSL/FastBlurMem.h>

namespace tav
{

class SNEnelPalabras  : public SceneNode
{
public:
    enum drawMode { RAW_DEPTH, DEPTH_THRESH, OPT_FLOW, FLUID, SHOW_WORD, CHAR_MAP, WORD_ACCUM, WORD_DIFF, DRAW };

	SNEnelPalabras(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	virtual ~SNEnelPalabras();

	void initShaders();
	void initSpriteShader();
	void initDrawInvShdr();
	void initAccumShdr();
	void initDrawAccumShdr();
	void initDiffShdr();
	void initGrayShdr();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void renderActWord(camPar* cp);
	void drawParticles(double time, double dt, camPar* cp);
	void update(double time, double dt);

	void onKey(int key, int scancode, int action, int mods);

private:
	FBO*					accumFbo;
	FBO*					charMapFbo;
	FBO*					charDrawFbo;
	FBO*					compFbo;
	FBO*					diffFbo;
	FBO*					textFbo;
	FBO*					threshFbo;

	FreetypeTex**			ft;

	GLSLParticleSystemFbo*	ps;
	GLSLParticleSystemFbo::EmitData data;
	GWindowManager*			winMan;
	GLSLHistogram*			histo;
	GLSLFluid*              fluidSim;
	GLSLOpticalFlow*		optFlow;

    FastBlurMem*            optFlowBlur;
    FastBlurMem*            fblur;
    FastBlurMem*            fblur2nd;

	Quad*					stdQuad;
	Quad*					stdHFlipQuad;
	Quad*					redQuad;

	Shaders*				accumShader;
    Shaders*                depthThres;
    Shaders*                depthThres2;
	Shaders*				colShader;
	Shaders*				drawAccumShader;
	Shaders*				diffShader;
	Shaders*				grayShader;
	Shaders*				invShader;
	Shaders*				mRenderProg;
	Shaders*				texShader;
	Shaders*				spriteShader;
	Shaders*				stdColAlphaShader;


	ShaderCollector*		shCol;
	//ShaderParams 			mShaderParams;
	TextureManager*			userMap;
	VAO*					initVAO;

	KinectInput*			kin;

	bool					wordInited=false;
	bool					firstCrossMax=false;
	bool					inited = false;
	bool					switching = false;

	unsigned int			maxNumParticles;
	unsigned int 			nrChars=25; // 6x6  felder
	unsigned int 			nrCharSide;
	unsigned int 			nrWords;
	unsigned int 			wordPtr=0;
	unsigned int			fblurSize;

	unsigned int 			charWidth;
	unsigned int 			charHeight;
	int						frameNr=-1;
	unsigned int 			flWidth;
	unsigned int 			flHeight;

	GLuint					mUBO;

    float					charAlpha=0.f;
    float					charFdbk;
    float					noiseFreq=10.f;
    float					noiseStren=0.001f;
    float					spriteSize=0.01f;
    float					initAmt=0.f;
    float					lifeTime;
    float					damping;
    float					velScale;
    float					nrEmitPart;
    float					velDirForceRel;
    float					charStep;
    float					accumAlpha;
    float					velPosForceRel;
    float					switchThres;
    float					userMapAlpha;

    float					timeMedFact;

    float					switchFadeTime;
    float					accumDrawAlpha=1.f;

    float fluidVelTexThres;
    float fluidVelTexRadius;
    float fluidVelTexForce;
    float fluidSmoke;
    float fluidVelDissip;

    float secondThresh;

    float					depthThresh;
    float 					fastBlurAlpha;
    float 					optFlowBlurAlpha;
    float 					contThresh;

    float					texNrElemStep;
    float					elemPerSide;
    float					emitThres;

	double					lastPartUpdt=0;
	double					lastEmit=0;
	float					emitIntrv=0.1;
	double					medDt=0;
	double					lastSwitch = 0;
	double 					lastTime = 0;

    float**         		DebugColors;
	unsigned int**			jointBufPtr;

	drawMode				actDrawMode;
};


} /* namespace tav */

#endif /* SRC_ENEL_SNENELPALABRAS_H_ */
