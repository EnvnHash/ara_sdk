//
// SNAudioWaveStrip2.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include "headers/gl_header.h"
#include <SceneNode.h>
#include "Meshes/MPQuad.h"
#include "GeoPrimitives/QuadArray.h"
#include <PAudio.h>
#include "Communication/OSC/OSCData.h"

namespace tav
{
class SNAudioWaveStrip2: public SceneNode
{
public:
	SNAudioWaveStrip2(sceneData* _scd,
			std::map<std::string, float>* _sceneArgs);
	~SNAudioWaveStrip2();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;
	void updateWave();
	void buildQuadSkeleton();
	void buildTriangles();
	void updateVAO();
private:
	ShaderCollector* shCol;
	OSCData* osc;
	PAudio* pa;
	QuadArray** quads;
	int lastBlock = -1;
	int nrLines;

	float* pllOffs;
	float* signals;
	float ampScale;
	float posOffs;
	float lineWidth;
	float minLineWidth;
	float waveAmpOffs;

	float alpha = 0.f;
	float aux1 = 0.f;
	float lineHeight = 0.f;

	int nrPar;

	glm::vec4* chanCols;
	glm::vec3** pllPositions;
	glm::vec3** pllNormals;

	glm::vec3** quadSkeleton;
	glm::vec3** quadNormal;
	glm::vec3** normSkeleton;

	glm::vec3** position;
	glm::vec3** normal;
	glm::vec2** texCoords;
};
}
