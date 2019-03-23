//
// SNTrustFdbk.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once


#include <iostream>

#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include "GLUtils/GLSL/FastBlurMem.h"
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include "GLUtils/GLSL/GLSLTimeMedian.h"
#include "GLUtils/PingPongFbo.h"
#include "VideoTextureCv.h"
#include <VideoTextureCvActRange.h>
#include <GLUtils/TextureManager.h>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

namespace tav
{

class SNTrustFdbk : public SceneNode
{
public:
	SNTrustFdbk(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTrustFdbk();

	void init(TFO* _tfo = nullptr);
    void initShdr();
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void startThreads(double time, double dt);
	void stopThreads(double time, double dt);
	void cleanUp();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};

private:
	Quad*                   quad;
	Quad*                   flipQuad;
    TextureManager*     	testTex;
    VideoTextureCvActRange* vt;

    ShaderCollector*				shCol;

	Shaders*            	stdTex;
    Shaders*            	maskShader;
    Shaders*            	applyMaskShader;
    Shaders*            	blendBlack;

    FastBlurMem*			blur;
    FastBlurMem*			blur2;
    GLSLTimeMedian*			tMed;
    GLSLTimeMedian*			tMed2;
    FBO*					finFbo;
    FBO*					medFbo;
    FBO*					smearFbo;
    PingPongFbo*			ppFbo;

    boost::mutex       		mutex;

	bool                    isInited = false;
	bool					optProcessing = false;

	unsigned int			vidStartOffs=0;
	unsigned int			actUplTexId=0;
	unsigned int			lastTexId=0;
	unsigned int			lastOptTexId=0;
	unsigned int			downscale=2;

	int 					optFrame=0;
	int						optLastFrame=0;


	float					median=136.f;
	float					flowFdbk=0.f;
	float					alpha=0.f;
	float					clearAlpha=0.1f;
	float					drawAlpha=1.f;
};
}
