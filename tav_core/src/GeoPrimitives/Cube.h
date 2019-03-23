//
//  CubeSubDiv.h
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <vector>
#include <utility>
#include "GeoPrimitives/GeoPrimitive.h"
#include "Meshes/MPQuad.h"
#include "GLUtils/glm_utils.h"

namespace tav
{
class Cube: public GeoPrimitive
{
public:
	Cube(float _width = 2.f, float _height = 2.f, float _depth = 2.f,
			int _nrSubDiv = 1, std::vector<coordType>* _instAttribs = nullptr,
			int _maxNrInstances = 1, float _r = 1.f, float _g = 1.f, float _b =
					1.f, float _a = 1.f, bool _invNormals = false,
			bool _smoothNor = false);
	Cube(std::vector<coordType>* _instAttribs, int _maxNrInstances, float _r =
			1.f, float _g = 1.f, float _b = 1.f, float _a = 1.f);
	~Cube();
	void init();
	void remove();
private:
	float width;
	float height;
	float depth;
	int nrSubDiv;
	bool invNormals;
	bool smoothNor;
	std::vector<std::pair<glm::vec3, float> > sides;
	std::vector<coordType>* instAttribs;
	int maxNrInstances = 1;
};
}
