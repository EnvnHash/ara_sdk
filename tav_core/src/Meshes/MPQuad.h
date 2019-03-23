//
//  MPQuad.h
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
class MPQuad: public MeshPrimitive
{
public:
	MPQuad();
	MPQuad(float x, float y, float w, float h);
	MPQuad(float x, float y, float w, float h, glm::vec3 inNormal, float _r =
			1.f, float _g = 1.f, float _b = 1.f, float _a = 1.f);
	~MPQuad();
	void init();
	void remove();
private:
	float width;
	float height;
};
}
