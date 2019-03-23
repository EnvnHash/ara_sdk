//
//  DiskSubDiv.h
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
#include "GLUtils/glm_utils.h"

namespace tav
{
class Disk: public GeoPrimitive
{
public:
	Disk(float _width = 2.f, float _height = 2.f, int _nrSubDiv = 1,
			std::vector<coordType>* _instAttribs = nullptr,
			int _maxNrInstances = 1, float _r = 1.f, float _g = 1.f, float _b =
					1.f, float _a = 1.f);
	~Disk();
	void init();
	void remove();
private:
	float width;
	float height;
	int nrSubDiv;
	std::vector<std::pair<glm::vec3, float> > sides;
	std::vector<coordType>* instAttribs;
	int maxNrInstances = 1;
};
}
