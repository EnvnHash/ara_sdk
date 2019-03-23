//
//  GUIAnimVal.h
//  tav_core
//
//  Created by Sven Hahne on 30/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef tav_core_AnimVal_h
#define tav_core_AnimVal_h

#pragma once

#include <functional>

namespace tav
{

enum AnimFunc
{
	RAMP_LIN_UP = 0, TRI_ONE_P = 1
};

class AnimValBase
{
public:
	virtual void update(double actTime)=0;
	virtual void stop()=0;
	virtual bool stopped()=0;
	virtual AnimFunc getAnimFunc()=0;
	virtual void setEndFunc(std::function<void()> _endFunc)=0;
};

template<typename T>
class AnimVal: public AnimValBase
{
public:
	AnimVal(AnimFunc _func, std::function<void()> _endFunc = nullptr) :
			func(_func), endFunc(_endFunc), run(false), delay(0.0)
	{}

	void update(double actTime)
	{
		if (run)
		{
			if (dur != 0.0)
			{
				perc = std::min(std::max(actTime - startTime - delay, 0.0) / dur, 1.0);
				switch (func)
				{
				case RAMP_LIN_UP:
					calcPerc = perc;

					break;
				case TRI_ONE_P:
					calcPerc = perc * 2.f;
					if (calcPerc > 1.f)
						calcPerc = 2.f - calcPerc;
					break;
				default:
					break;
				}

				if (perc >= 1.0)
				{
					run = false;
					if (endFunc)
						endFunc();
					if (loop)
					{
						perc = 0;
						calcPerc = 0;

						startTime = actTime;
						run = true;
					}
				}
			}
			else
			{
				printf("tav::AnimVal Error: duration is 0\n");
			}
		}
	}

	void start(T _startVal, T _endVal, double _dur, double _actTime, bool _loop)
	{
		run = true;
		startTime = _actTime;
		startVal = _startVal;
		endVal = _endVal;
		dur = _dur;
		loop = _loop;
	}

	void stop(){
		run = false;
		if (endFunc) endFunc();
	}

	bool stopped() {
		return !run;
	}

	AnimFunc getAnimFunc() {
		return func;
	}

	void setEndFunc(std::function<void()> _endFunc) {
		endFunc = _endFunc;
	}

	float getPercentage() {
		return perc;
	}

	void reset(){
		perc = 0;
		calcPerc = 0;
	}

	T getVal(){
		return (startVal * (1.0 - calcPerc)) + endVal * calcPerc;
	}

	void setInitVal(T _val){
		startVal = _val;
	}

	void setVal(T _val){
		startVal = _val;
		run = false;
		calcPerc = 0.f;
	}

	void setDelay(double _val){
		delay = _val;
	}

private:
	T startVal;
	T endVal;
	AnimFunc func;
	bool run;
	bool loop = false;
	double dur = 0.0;
	double startTime;
	double delay;
	float perc = 0;
	float calcPerc = 0;
	std::function<void()> endFunc;
};

}

#endif
