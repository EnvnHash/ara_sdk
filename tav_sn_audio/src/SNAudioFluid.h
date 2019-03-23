//
// SNAudioFluid.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include "GLUtils/GLSL/GLSLFluid.h"
#include "GeoPrimitives/QuadArray.h"
#include "GeoPrimitives/Quad.h"
#include "Communication/OSC/OSCData.h"
#include <PAudio.h>
#include "Shaders/ShaderCollector.h"

namespace tav
{
class SNAudioFluid: public SceneNode
{
public:
	SNAudioFluid(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNAudioFluid();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;
private:
	bool inited = false;

	PAudio* pa;
	OSCData* osc;
	GLSLFluid* fluidSim;
	QuadArray* quadAr;
	Quad* quad;
	ShaderCollector* shCol;
	Shaders* stdTexAlpha;

	glm::vec4* chanCols;
	glm::vec2* oldPos;
	glm::vec2 forceScale;

	float alpha = 0.f;
	float alphaScale;
	float posScale;
	float eRad = 3.f;
	float eTemp = 10.f;
	float eDen = 1.f;
	float ampScale;
	float timeStep = 0.25f;
	float velDiss = 0.99f;

	GLint drawTex = 0;
	unsigned int flWidth;
	unsigned int flHeight;
	int lastBlock = -1;
};
}
