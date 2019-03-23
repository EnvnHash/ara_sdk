/*
 *  Timer.h
 *  ta_visualizer
 *
 *  Created by Sven Hahne on 11.01.11.
 *  Copyright 2011 Sven Hahne. All rights reserved..
 *
 */

#include <sys/time.h> /* Linux system clock functions */
#include <time.h>

#ifndef _TIMER_
#define _TIMER_

class Timer
{

public:
	Timer();
	~Timer();
	void saveCurrentTimeAsStart();
	void saveCurrentTimeAsEnd();
	double getCurrentTime();
	double getCurrentDt();
	double getDt();
	struct timespec *getUsec(double dt);
	double startTime;
	double endTime;
	struct timespec req;

	/*
	 double destFrameTime = 1.0 / (75.0 * 0.5);
	 int nrFrameSamples[3];
	 double startFrameTime = 0.0;
	 double startCtrFrameTime = 0.0;
	 double finishedFrameTime = 0.0;
	 int showFrameRate = 0;
	 */
};

#endif
