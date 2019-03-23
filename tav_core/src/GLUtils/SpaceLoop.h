//
//  SpaceLoop.h
//  tav_gl4
//
//  Created by Sven Hahne on 04.09.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//
#pragma once

#include <cmath>

namespace tav
{
class SpaceLoop
{
public:
	SpaceLoop(int _nrItems, float _scale, float _offs, float _speed);
	~SpaceLoop();
	void update(double dt);
	float getPos(int ind);
	void setSpeed(float _speed);
private:
	int nrItems;

	double lastTime;

	float dt = 0.f;
	float incrTime;
	float speed;
	float* positions;
	float scale;
	float offs;
};
}
