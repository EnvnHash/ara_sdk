//
// SNAudioRingV.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <cmath>

#include <GeoPrimitives/Line.h>
#include "GeoPrimitives/Quad.h"
#include "GeoPrimitives/QuadArray.h"
#include <Communication/OSC/OSCData.h>
#include <PAudio.h>
#include "VideoTextureCv.h"
#include <SceneNode.h>

namespace tav
{
class SNAudioRingV: public SceneNode
{
public:
	SNAudioRingV(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNAudioRingV();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{}
	void updateWave();
	void updateGeo();
	void updateVAO(double _dt);
private:
	VideoTextureCv* vt;
	PAudio* pa;
	OSCData* osc;
	ShaderCollector* shCol;

	QuadArray** qArrays;
	int lastBlock = -1;
	int nrXSegments;
	int upperLower;

	glm::vec3* pllOffs;
	float ampScale;
	float basicScale;
	float yDistScale;
	float posOffs;
	float ringHeight;
	float flipNormals;
	float zOffs;
	float yDist;
	float rotSpeed;
	float dt;
	float incTime;
	float ringWidth;

	double lastTime = -1.f;
	glm::mat4* modMatrPerChan;

	glm::vec4* chanCols;
	glm::vec3*** pllPositions;
	glm::vec3*** pllNormals;

	glm::vec3** position;
	glm::vec3** normal;
};
}
