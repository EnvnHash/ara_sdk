//
// SNAudioWaveSlide.h
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
#include "GLUtils/PingPongFbo.h"
#include "Shaders/ShaderCollector.h"
#include "GeoPrimitives/Line.h"
#include "Communication/OSC/OSCData.h"

namespace tav
{
class SNAudioWaveSlide: public SceneNode
{
public:
	SNAudioWaveSlide(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNAudioWaveSlide();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void initPreCalcShdr();
	void initTex1DShdr();
	void initWaveShdr(TFO* _tfo);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;
private:
	bool inited = false;

	TextureManager* tex0;
	OSCData* osc;
	PAudio* pa;
	Quad* quad;
	QuadArray* quadAr;

	ShaderCollector* shCol;
	Shaders* posTexShdr;
	Shaders* normTexShdr;
	Shaders* waveShdr;
	Shaders* stdTex;
	Shaders* stdTex2D;

	FBO* posTex;
	FBO* normTex;
	PingPongFbo* pllSmooth;
	glm::vec4* chanCols;

	double lastTime;
	int lastBlock = -1;
	unsigned int texGridSize;

	glm::mat4 modelMat;
	glm::mat3 normalMat;
};
}
