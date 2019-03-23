//
// SNParaColumns.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <cmath>
#include <SceneNode.h>
#include "GeoPrimitives/Cube.h"
#include <PAudio.h>

namespace tav
{
class SNParaColumns: public SceneNode
{
public:
	typedef struct
	{
		float on = 0.0f;
		float height = 0.f;
		float width = 0.f;
		float depth = 0.f;
		float zPos = 0.0f;
		float xOffs = 0.0f;
		float amp = 0.0f;
		float texOffsX = 0.0f;
		float texOffsY = 0.0f;
	} cubePar;

	SNParaColumns(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNParaColumns();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);
private:
	ShaderCollector* shCol;
	Cube* cubes;
	cubePar* cubePars;
	PAudio* pa;
	TextureManager* tex0;

	int nrCubes;
	int cubesPerChan;
	int maxNrChans = 4;
	int lastBlock = -1;

	float depthScale;
	float yOffs;
	float speed;
	glm::vec4* chanCols;
	glm::vec3* chanSpaceMap;
	float sizeScale;
};
}
