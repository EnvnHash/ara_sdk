//
//  MPCircle2Sides.h
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//
//

#pragma once

#include <iostream>
#include <vector>
#include "MeshPrimitive.h"

#ifndef TWO_PI
#define TWO_PI 6.28318530717958647693
#endif

namespace tav
{
class MPCircle2Sides: public MeshPrimitive
{
public:
	MPCircle2Sides();
	MPCircle2Sides(int _nrSegs, float _outerRad, float _innerRad, float _angle =
			TWO_PI, float _r = 1.f, float _g = 1.f, float _b = 1.f, float _a =
			1.f);
	~MPCircle2Sides();

	void init();
	void remove();
	int getNrSegments();
private:
	bool closeCircle;
	int nrSegQuads;
	float outerRadius;
	float innerRadius;
	float angle;
	float smoothNorm;

	std::vector<glm::vec3> outerRing;
	std::vector<glm::vec3> innerRing;
};
}
