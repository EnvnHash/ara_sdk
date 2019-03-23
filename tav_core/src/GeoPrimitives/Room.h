//
//  Room.h
//  tav_gl4
//
//  Created by Sven Hahne on 16.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include "GeoPrimitives/GeoPrimitive.h"
//#include "ofPlane.h"

namespace tav
{
class Room: public GeoPrimitive
{
public:
	Room(float _width, float _height, float _depth);
	Room(float _width, float _height, float _depth, int _nrSubDiv);
	~Room();
	void init();
	void draw(TFO* _tfo = nullptr);
	void draw(GLenum _type, TFO* _tfo = nullptr);
	void remove();
private:
	int nrSubDiv;
	float width;
	float height;
	float depth;
//        ofPlane**       walls;
};
}
