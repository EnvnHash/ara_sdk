//
//  SpaceLoop.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 04.09.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "GLUtils/SpaceLoop.h"

namespace tav
{
SpaceLoop::SpaceLoop(int _nrItems, float _scale, float _offs, float _speed) :
		nrItems(_nrItems), speed(_speed), offs(_offs), scale(_scale)
{
	lastTime = -1.f;
	incrTime = 0.f;
	positions = new float[nrItems];
}

//------------------------------------------------------------------------------------------------

SpaceLoop::~SpaceLoop()
{
	delete positions;
}

//------------------------------------------------------------------------------------------------

void SpaceLoop::update(double dt)
{
	incrTime += dt * speed;
	if (incrTime < 0.f)
		incrTime = 1.f;

	for (auto i = 0; i < nrItems; i++)
	{
		float fInd = static_cast<float>(i) / static_cast<float>(nrItems);
		positions[i] = std::fmod(fInd + (incrTime), 1.0f) * scale + offs;
	}
}

//------------------------------------------------------------------------------------------------

float SpaceLoop::getPos(int ind)
{
	return positions[ind];
}

//------------------------------------------------------------------------------------------------

void SpaceLoop::setSpeed(float _speed)
{
	speed = _speed;
}
}
