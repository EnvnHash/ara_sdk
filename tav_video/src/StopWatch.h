/*
 * StopWatch.h
 *
 *  Created on: Jul 18, 2018
 *      Author: sven
 */

#ifndef SRC_STOPWATCH_H_
#define SRC_STOPWATCH_H_


#include <chrono>


class StopWatch
{
public:
	StopWatch() : showDecIt(0), dt(0.0), showDecTimeInt(80) {}
	~StopWatch() {};

	void setStart(){
    	start = std::chrono::system_clock::now();
	}

	void setEnd(){
    	end = std::chrono::system_clock::now();
    	auto diff = end - start;
    	actDifF = std::chrono::duration <double, std::milli> (diff).count();
	}

	void print(char* pre){
    	if (dt == 0.0)
    		dt = actDifF;
    	else
    		dt = ((dt * 3.0) + actDifF) / 4.0;

    	if (showDecIt >= showDecTimeInt){
    		printf("%s dt time %f\n", pre, dt);
    		showDecIt=0;
    	}
    	showDecIt++;
	}

	std::chrono::time_point<std::chrono::system_clock> start;
	std::chrono::time_point<std::chrono::system_clock> end;

	double dt;
	double actDifF;
	int showDecIt;
	int showDecTimeInt;

};

#endif /* STOPWATCH_H_ */
