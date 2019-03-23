//
//  MPCubeElem.h
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

namespace tav
{
class MPCubeElem: public MeshPrimitive
{
public:
	MPCubeElem(float _width = 1.f, float _height = 1.f, float _depth = 1.f,
			float _r = 1.f, float _g = 1.f, float _b = 1.f, float _a = 1.f);
	~MPCubeElem();
	void remove();
private:
	float width;
	float height;
	float depth;
};
}
