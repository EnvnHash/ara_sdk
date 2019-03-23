//
// SNAudioWaveStrip.h
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

namespace tav
{
class SNAudioWaveStrip: public SceneNode
{
public:
	SNAudioWaveStrip(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNAudioWaveStrip();

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
	void updateVAO(double time);
private:
	ShaderCollector* shCol;
	PAudio* pa;
	QuadArray** quads;
	TextureManager* tex0;
	int lastBlock = -1;
	int nrLines;

	float* pllOffs;
	float* signals;
	float ampScale;
	float posOffs;
	float lineWidth;
	float waveAmpOffs;

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
