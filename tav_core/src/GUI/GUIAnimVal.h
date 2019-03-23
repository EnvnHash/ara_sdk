//
//  GUIAnimVal.h
//  tav_core
//
//  Created by Sven Hahne on 30/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef tav_core_GUIAnimVal_h
#define tav_core_GUIAnimVal_h

#pragma once

#include <functional>

namespace tav
{
typedef std::function<void()> GUICallbackFunction;

enum guiAnimFunc
{
	GUI_HIGHLIGHT = 0,
	GUI_BLINK_ONCE = 1,
	GUI_MOVE_RIGHT = 2,
	GUI_MOVE_LEFT = 3,
	GUI_MOVE_UP = 4,
	GUI_MOVE_DOWN = 5
};
enum guiAnimTimeFunc
{
	GUI_RAMP_LIN_UP = 0, GUI_TRI_ONE_P = 1
};

class GuiAnimValBase
{
public:
	virtual void update(float perc)=0;
	virtual guiAnimFunc getAnimFunc()=0;
	virtual void start()=0;
	virtual void stop()=0;
	virtual bool stopped()=0;
};

template<typename T>
class GuiAnimVal: public GuiAnimValBase
{
public:
	GuiAnimVal(guiAnimFunc _func, T* _startVal, T* _endVal,
			GUICallbackFunction _endFunc) :
			func(_func), updtVal(_startVal), endVal(_endVal), endFunc(_endFunc)
	{
		startVal = *_startVal;
	}
	void update(float perc)
	{
		guiAnimTimeFunc fn = GUI_RAMP_LIN_UP;

		switch (func)
		{
		case GUI_HIGHLIGHT:
			fn = GUI_RAMP_LIN_UP;
			break;
		case GUI_BLINK_ONCE:
			fn = GUI_TRI_ONE_P;
			break;
		case GUI_MOVE_RIGHT:
			fn = GUI_RAMP_LIN_UP;
			break;
		case GUI_MOVE_LEFT:
			fn = GUI_RAMP_LIN_UP;
			break;
		case GUI_MOVE_UP:
			fn = GUI_RAMP_LIN_UP;
			break;
		case GUI_MOVE_DOWN:
			fn = GUI_RAMP_LIN_UP;
			break;
		default:
			break;
		}

		switch (fn)
		{
		case GUI_RAMP_LIN_UP:
			*updtVal = startVal * (1.f - perc) + (*endVal * perc);
			break;
		case GUI_TRI_ONE_P:
			perc = perc * 2.f;
			if (perc > 1.f)
				perc = 2.f - perc;
			*updtVal = startVal * (1.f - perc) + (*endVal * perc);
			break;
		default:
			break;
		}
	}
	void start()
	{
		run = true;
	}
	void stop()
	{
		run = false;
		if (endFunc)
			endFunc();
	}
	bool stopped()
	{
		return !run;
	}
	guiAnimFunc getAnimFunc()
	{
		return func;
	}
	;
private:
	T* updtVal;
	T startVal;
	T* endVal;
	guiAnimFunc func;
	bool run;
	GUICallbackFunction endFunc;
};

}

#endif
