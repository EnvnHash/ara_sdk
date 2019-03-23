//
// SNAudioRing.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include "GeoPrimitives/Line.h"
#include "GeoPrimitives/Quad.h"
#include "GeoPrimitives/QuadArray.h"
#include <PAudio.h>
#include "Communication/OSC/OSCData.h"

namespace tav
{
class SNAudioRing: public SceneNode
{
public:
	SNAudioRing(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNAudioRing();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;
	void updateWave();
	void updateGeo();
	void updateVAO(double _dt);
private:
	PAudio* pa;
	OSCData* osc;
	QuadArray** qArrays;
	TextureManager* tex0;
	ShaderCollector* shCol;

	int lastBlock = -1;
	int nrXSegments;
	int upperLower;

	glm::vec4* chanCols;
	glm::vec3* pllOffs;

	float ampScale;
	float basicScale;
	float yDistScale;
	float posOffs;
	float ringHeight;
	float flipNormals;
	float zOffs;
	float rotSpeed;
	float dt;
	float incTime;

	float alpha = 1.f;
	float aux1 = 0.9f;
	float yDist = 0.6f;
	float lineHeight = 0.3f;

	double lastTime = -1.f;
	glm::mat4* modMatrPerChan;

	glm::vec3*** pllPositions;
	glm::vec3*** pllNormals;

	glm::vec3** position;
	glm::vec3** normal;
};
}
