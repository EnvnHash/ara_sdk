//
// SNCircleOrbit2.h
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
#include "GLUtils/SpaceLoop.h"

namespace tav
{

class SNCircleOrbit2: public SceneNode
{
public:
	typedef struct
	{
		glm::vec3 pos;
		float rotZ;
	} circPar;

	SNCircleOrbit2(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNCircleOrbit2();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;
	void updateModMatr(float dt);

private:
	ShaderCollector* shCol;
	Circle** circs;
	circPar** cp;
	OSCData* osc;
	PAudio* pa;
	SpaceLoop** sLoop;
	glm::vec4* chanCols;
	TextureManager* tex0;

	float yAmp;
	float circBaseSize;
	float rotSpeed;
	float depthScale;
	float partOffs;
	float scaleAmt;
	float bright;

	int nrPartCircs;
	int nrInst;
	int lastBlock = -1;
};
}
