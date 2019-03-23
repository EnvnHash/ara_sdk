//
//  Circle.h
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//
//

#pragma once

#include <iostream>
#include <vector>
#include "GeoPrimitives/GeoPrimitive.h"
#include "../Meshes/MPCircle.h"
#include "GLUtils/glm_utils.h"

namespace tav
{
class Circle: public GeoPrimitive
{
public:
	Circle();
	Circle(int _nrSegs, float _outerRad, float _innerRad, float _angle = TWO_PI,
			float _r = 1.f, float _g = 1.f, float _b = 1.f, float _a = 1.f,
			std::vector<coordType>* _instAttribs = nullptr,
			int _nrInstances = 1);
	~Circle();
	void init();
	void remove();
private:
	MPCircle* circ;

	std::vector<coordType>* instAttribs;
	int maxNrInstances = 1;
};
}
