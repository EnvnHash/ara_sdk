//
// SNAudioTunnel.h
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
#include "GeoPrimitives/QuadArray.h"
#include "math_utils.h"
#include <PAudio.h>
#include "Communication/OSC/OSCData.h"

namespace tav
{
class SNAudioTunnel: public SceneNode
{
public:
	SNAudioTunnel(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNAudioTunnel();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;
	void updateWave(int mixDownChanInd, int pllInd);
	void updateGeo();
	void updateVAO(double _time);
private:
	PAudio* pa;
	QuadArray** quadArrays;
	OSCData* osc;
	ShaderCollector* shCol;

	int nrXSegments;
	int nrYSegments;
	int nrPllSnapShots;
	int nrPllSnapShotsC;
	int lastBlock = -1;

	float ampScale;
	float basicRadius;
	float posOffs;
	float ringHeight;
	float flipNormals;
	float zOffs;
	float yStepSize;
	float speed;
	float rotPerChan;
	float bright;
	float dt = 0.f;
	float incTime = 0.f;

	double lastTime;

	glm::vec3 pllOffs;
	glm::mat4* modMatrPerChan;
	glm::vec4* chanCols;

	float** pllTimeOffs;
	glm::vec3*** pllPositions;
	glm::vec3**** pllNormals;
	glm::vec3*** position;

	int mixDownToNrChans;
	std::vector<short>* chanDownMix;
};
}
