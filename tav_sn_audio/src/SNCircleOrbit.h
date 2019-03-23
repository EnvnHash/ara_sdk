//
// SNCircleOrbit.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include "GeoPrimitives/Circle.h"
#include "GLUtils/SpaceLoop.h"
#include "math_utils.h"
#include <PAudio.h>
#include "Communication/OSC/OSCData.h"

namespace tav
{
class SNCircleOrbit: public SceneNode
{
public:
	typedef struct
	{
		glm::vec3 pos;
		float rotZ;
	} circPar;

	SNCircleOrbit(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNCircleOrbit();

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
	PAudio* pa;
	OSCData* osc;
	SpaceLoop** sLoop;
	TextureManager* tex0;

	glm::vec4* chanCols;

	float yAmp;
	float circBaseSize;
	float rotSpeed;
	float depthScale;
	float dt = 0.f;
	float partOffs;
	float scaleAmt;
	float bright;
	float alpha = 0.f;

	int nrPartCircs;
	int nrInst;
	int lastBlock = -1;
};
}
