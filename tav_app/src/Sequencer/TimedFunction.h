/*
 * TimedFunction.h
 *
 *  Created on: 23.02.2017
 *      Copyright by Sven Hahne
 */

#ifndef SEQUENCER_TIMEDFUNCTION_H_
#define SEQUENCER_TIMEDFUNCTION_H_

#pragma once

#include <functional>
#include <chrono>
#include <thread>             // std::thread
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable
#include <cstdio>

namespace tav
{
typedef std::function<void()> EventCallBack;

class TimedFunction
{
public:
	TimedFunction(unsigned int execAfter, EventCallBack _f)
	{
		t1 = std::thread(&TimedFunction::wait, this, execAfter, _f);
		t1.detach();
	}

	void wait(unsigned int waitNanoSec, EventCallBack _f)
	{
		std::unique_lock<std::mutex> lck(mtx);
		cv.wait_for(lck, std::chrono::milliseconds(waitNanoSec));
		if (exec)
			_f();
	}

	void abort()
	{
		exec = false;
		cv.notify_all();
	}

	bool exec = true;
	std::condition_variable cv;
	std::mutex mtx;
	std::thread t1;
};
}

#endif /* SEQUENCER_TIMEDFUNCTION_H_ */
