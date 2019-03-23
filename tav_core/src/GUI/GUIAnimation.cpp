//
//  GUIAnimation.cpp
//  tav_core
//
//  Created by Sven Hahne on 30/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "GUIAnimation.h"

namespace tav
{
GUIAnimation::GUIAnimation() :
		anTimeShape(GUI_LINEAR), duration(1.0), run(false)
{
}

//--------------------------------------------------

void GUIAnimation::start()
{
	//printf("GUIAnimation::start \n");

	elapsedTime = 0;
	run = true;
	for (std::vector<GuiAnimValBase*>::iterator it = valMap.begin();
			it != valMap.end(); ++it)
		(*it)->start();
}

//--------------------------------------------------

void GUIAnimation::update(double time, double dt)
{
	bool noAnimRunning = true;
	elapsedTime += dt;

	// update all values that were pushed back
	for (std::vector<GuiAnimValBase*>::iterator it = valMap.begin();
			it != valMap.end(); ++it)
	{
		if (elapsedTime <= durMap[(*it)->getAnimFunc()])
		{
			noAnimRunning = false;

			// get percentage of elapsed Time
			float percTime = std::fmin(
					elapsedTime / durMap[(*it)->getAnimFunc()], 1.f);
			(*it)->update(percTime);
		}
		else
		{
			if (!(*it)->stopped())
				(*it)->stop();
		}
	}

	if (noAnimRunning)
	{
		run = false;
		valMap.clear();
	}
}

//--------------------------------------------------

bool GUIAnimation::isRunning()
{
	return run;
}

//--------------------------------------------------

void GUIAnimation::setAnimFunc(guiAnimFunc _anFunc)
{
	anFunc = _anFunc;
}

//--------------------------------------------------

void GUIAnimation::setDuration(guiAnimFunc _anFunc, float _dur)
{
	durMap[_anFunc] = _dur;
}

//--------------------------------------------------

void GUIAnimation::setTimeShape(guiAnimTimeShape _shape)
{
	anTimeShape = _shape;
}

//--------------------------------------------------

GUIAnimation::~GUIAnimation()
{
}
}
