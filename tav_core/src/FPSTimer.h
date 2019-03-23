//
//  FPSTimer.h
//
//  Created by Sven Hahne on 04.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#ifndef Test_Basic_GL4_FPSTimer_h
#define Test_Basic_GL4_FPSTimer_h

#include <thread>
#include <boost/timer/timer.hpp>

using boost::timer::cpu_timer;
using boost::timer::cpu_times;
using boost::timer::nanosecond_type;

class FPSTimer
{
public:
	FPSTimer() :
			checkInterv(2 * 1000000000LL), last(0)
	{
	}
	void checkTime()
	{
		cpu_times const elapsed_runTimes(runTimer.elapsed());
		cpu_times const elapsed_fpsTimes(fpsTimer.elapsed());
		dt = static_cast<double>(elapsed_fpsTimes.wall * 0.000000001);
		runTimeElapsed =
				static_cast<double>(elapsed_runTimes.wall * 0.000000001);

		if (printFps == true)
		{
			if (lastFpsTime == 0.0)
			{
				lastFpsTime = dt;
			}
			else
			{
				lastFpsTime = (lastFpsTime * 20.0 + dt) / 21.0;
			}

			if (elapsed_runTimes.wall - last >= checkInterv)
			{
				std::cout << "dt: " << lastFpsTime << " fps: "
						<< 1.0 / lastFpsTime << std::endl;
				last = elapsed_runTimes.wall;
			}

			fpsTimer.stop();
			fpsTimer.start();
		}
	}
	void showFps(bool _set)
	{
		printFps = _set;
	}
	double getTimeElapsed()
	{
		return runTimeElapsed;
	}

private:
	nanosecond_type const checkInterv;
	nanosecond_type last;

	bool printFps = false;

	cpu_timer fpsTimer;
	cpu_timer runTimer;
	double lastFpsTime = 0.0;
	double dt;
	double medDt;
	double runTimeElapsed = 0.0;
};
#endif
