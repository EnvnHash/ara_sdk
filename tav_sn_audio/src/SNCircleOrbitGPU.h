//
// SNCircleOrbitGPU.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include "headers/gl_header.h"
#include <SceneNode.h>
#include "GeoPrimitives/Circle.h"
#include <PAudio.h>
#include "Communication/OSC/OSCData.h"
#include "Shaders/Shaders.h"

namespace tav
{

class SNCircleOrbitGPU: public SceneNode
{
public:
	SNCircleOrbitGPU(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNCircleOrbitGPU();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void initRecShdr(TFO* _tfo);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;

private:
	ShaderCollector* shCol;
	Circle* circs;
	OSCData* osc;
	PAudio* pa;
	Shaders* recShdr;

	glm::vec4* chanCols;

	float yAmp;
	float circBaseSize;
	float rotSpeed;
	float depthScale;
	float partOffs;
	float scaleAmt;
	float bright;
	float timeIncr = 0;

	float audioTex2DNrChans;

	int nrPartCircs;
	int nrInst;
	int lastBlock = -1;

	bool inited;
};
}
