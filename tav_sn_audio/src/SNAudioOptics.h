//
// SNAudioOptics.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <utility>
#include <cmath>
#include <SceneNode.h>
#include "GeoPrimitives/Quad.h"
#include "math_utils.h"
#include <PAudio.h>
#include "Communication/OSC/OSCData.h"
#include "GeoPrimitives/Disk.h"
#include "GLUtils/PingPongFbo.h"
#include "GLUtils/PropoImage.h"

namespace tav
{
class SNAudioOptics: public SceneNode
{
public:
	SNAudioOptics(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNAudioOptics();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void initShader();
	void initTex1DShdr();
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;
private:
	PAudio* pa;
	OSCData* osc;
	Disk* disk;
	Quad* quad;

	ShaderCollector* shCol;
	Shaders* stdCol;
	Shaders* stdTex;
	Shaders* stdTex2D;

	PingPongFbo* pllSmooth;
	PropoImage* ikinLogo;

	glm::vec4* chanCols;

	double lastTime;
	double timeScale;

	int nrInst;
	int nrSegments;

	float offsetAmp = 1.5f;
	float audioAmp = 2.47f;
	float audioMed = 0.5f;
	float baseScale = 1.87f;
	float alpha = 0.f;
};
}
