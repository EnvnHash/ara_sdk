//
// SNAudioPlane.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include "GeoPrimitives/QuadArray.h"
#include "GeoPrimitives/Quad.h"
#include <PAudio.h>
#include "GLUtils/FBO.h"
#include "Shaders/ShaderCollector.h"
#include "GeoPrimitives/Line.h"
#include "Communication/OSC/OSCData.h"

namespace tav
{
class SNAudioPlane: public SceneNode
{
public:
	SNAudioPlane(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNAudioPlane();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void initPreCalcShdr();
	void initWaveShdr(TFO* _tfo);
	void initRenderShdr();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{}
private:
	bool inited = false;

	OSCData* osc;
	PAudio* pa;
	Quad* quad;
	QuadArray* quadAr;

	ShaderCollector* shCol;
	Shaders* posTexShdr;
	Shaders* normTexShdr;
	Shaders* waveShdr;
	Shaders* renderShdr;
	Shaders* stdTex;

	TextureManager* litsphereTex;
	TextureManager* bumpMap;
	TextureManager* cubeTex;
	TextureManager* tex0;

	FBO* posTex;
	FBO* normTex;

	double lastTime;
	int lastBlock = -1;
	unsigned int texGridSize;

	float alpha = 0.f;
	float heightScale = 1.f;

	glm::vec4* chanCols;
	glm::mat4 waveRotModelMat;
	glm::mat3 normalMat;
};
}
