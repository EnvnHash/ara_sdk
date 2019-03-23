/*
 *  Timer.cpp
 *  ta_visualizer
 *
 *  Created by Sven Hahne on 11.01.11.
 *  Copyright 2011 Sven Hahne. All rights reserved.
 *
 */

#include "pch.h"
#include "Timer.h"
#include <iostream>

Timer::Timer()
{
	startTime = getCurrentTime();
	//std::cout << "startTime: " << startTime << std::endl;
}

Timer::~Timer()
{
}

void Timer::saveCurrentTimeAsStart()
{
	startTime = getCurrentTime();
}
void Timer::saveCurrentTimeAsEnd()
{
	endTime = getCurrentTime();
}

double Timer::getCurrentTime()
{
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	return (double) tv.tv_sec + (double) tv.tv_usec / (1000 * 1000);
}

double Timer::getDt()
{
	return endTime - startTime;
}

double Timer::getCurrentDt()
{
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	return ((double) tv.tv_sec + (double) tv.tv_usec / (1000 * 1000))
			- startTime;
}

struct timespec *Timer::getUsec(double dt)
{
	int usec;

	// convert to usec
	usec = (int) (dt * 1000000.0);
	req.tv_sec = (int) usec / 1000000;
	req.tv_nsec = (int) (usec % 1000000) * 1000;
	return (&req);
}
