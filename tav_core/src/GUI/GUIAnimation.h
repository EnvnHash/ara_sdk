//
//  GUIAnimation.h
//  tav_core
//
//  Created by Sven Hahne on 30/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <stdio.h>
#include <iostream>

#include <map>
#include <vector>
#include <glm/glm.hpp>

#include "GUIAnimVal.h"

namespace tav
{

class GUIAnimation
{
public:
	enum guiAnimTimeShape
	{
		GUI_LINEAR = 0, GUI_EXP = 1
	};

	GUIAnimation();
	~GUIAnimation();

	void start();
	void update(double time, double dt);

	bool isRunning();

	void setAnimFunc(guiAnimFunc _anFunc);
	void setDuration(guiAnimFunc _anFunc, float _dur);
	void setTimeShape(guiAnimTimeShape _shape);

	//--------------------------------------------------

	template<typename T>
	void setAnimVal(guiAnimFunc _func, T* _fromVal, T* _toVal,
			GUICallbackFunction _endFunc = 0)
	{
		if (!run)
			valMap.push_back(
					new GuiAnimVal<T>(_func, _fromVal, _toVal, _endFunc));
	}

private:
	guiAnimFunc anFunc;
	guiAnimTimeShape anTimeShape;

	bool run;

	double duration;
	double elapsedTime;

	std::map<guiAnimFunc, float> durMap;
	std::vector<GuiAnimValBase*> valMap;
};
}
