//
//  SingleQuad.h
//  tav_gl4
//
//  Created by Sven Hahne on 07.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include "GeoPrimitives/GeoPrimitive.h"

namespace tav
{
class SingleQuad: public GeoPrimitive
{
public:
	SingleQuad();
	SingleQuad(float _x, float _y, float _width, float _height);
	SingleQuad(float _x, float _y, float _width, float _height, float _r,
			float _g, float _b, float _a);
	SingleQuad(float _x, float _y, float _width, float _height, float _r,
			float _g, float _b, float _a, bool _flipH, bool _flipV);
	~SingleQuad();
	void init();
	void draw();
	void draw(GLenum _type);
	void remove();
private:
	float x;
	float y;
	float width;
	float height;
	bool flipH = false;
	bool flipV = false;
};
}
