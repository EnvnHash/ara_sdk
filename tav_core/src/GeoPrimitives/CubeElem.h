//
//  CubeElem.h
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
#include "Meshes/MPCubeElem.h"
#include "GLUtils/glm_utils.h"

namespace tav
{
class CubeElem: public GeoPrimitive
{
public:
	CubeElem(float _width = 1.f, float _height = 1.f, float _depth = 1.f);
	CubeElem(float _width, float _height, float _depth,
			std::vector<coordType>* _instAttribs, int _maxNrInstances,
			float _r = 1.f, float _g = 1.f, float _b = 1.f, float _a = 1.f);
	~CubeElem();
	void draw(TFO* _tfo = nullptr);
	void init();
	void remove();
private:
	MPCubeElem* mCubeElem;
	float width;
	float height;
	float depth;
	std::vector<coordType>* instAttribs;
	int maxNrInstances = 1;
};
}
