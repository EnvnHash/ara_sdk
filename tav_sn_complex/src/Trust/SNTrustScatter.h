//
// SNTrustScatter.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once


#include <iostream>

#include "GeoPrimitives/Circle.h"
#include "GLUtils/GLSL/FastBlurMem.h"
#include <GLUtils/FBO.h>
#include "GLUtils/GLSL/GLSLTimeMedian.h"
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include <GeoPrimitives/Quad.h>
#include <SceneNode.h>
#include <VideoTextureCvActRange.h>
#include <GLUtils/GLSL/GodRays.h>


namespace tav
{

class SNTrustScatter : public SceneNode
{
public:
	SNTrustScatter(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTrustScatter();

	void init(TFO* _tfo = nullptr);
    void initShdr();
    void initGodRayShdr();
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void startThreads(double time, double dt);
	void stopThreads(double time, double dt);
	void cleanUp();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};

private:
	FBO*					godRaysFbo;
	FBO*					maskFbo;
    FastBlurMem*			blur;
    GLSLTimeMedian*			tMed;
    GLSLOpticalFlow*		optFlow;
	GodRays*				godRays;

	Quad*                   quad;
	Quad*                   quadNoFlip;
	Circle*					lightCircle;

    ShaderCollector*				shCol;
	Shaders*            	godRayShdr;
	Shaders*            	stdColShdr;
	Shaders*            	stdTex;
	Shaders*            	maskShader;
	Shaders*            	applyMaskShader;

	VideoTextureCvActRange* vt;

    glm::vec3 				lightPos;

	int						actUplTexId;
	int						lastTexId;

    float				alpha=0.f;
	float				decay=0.9999f;
	float				density=0.084f;
	float				exposure=0.01f;
	float				voluNetAlpha=0.f;
	float				lightX=0.f;
	float				lightY=0.f;
	float				grAlpha=0.5f;

};
}
