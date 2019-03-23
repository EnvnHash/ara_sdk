//
//  TrackObj.h
//  BlobTracking
//
//  Created by Sven Hahne on 16.06.14.
//  Copyright (c) 2014 _Sven Hahne. All rights reserved..
//

#ifndef __BlobTracking__TrackObj__
#define __BlobTracking__TrackObj__

#include <iostream>
#include <cstdio>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <fstream>
#include <map>
#include <mutex>
#include <stdio.h>
#include <sys/stat.h>

#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/string_cast.hpp>

#include <OpenCvUtils/CvKalmanFilter.h>
#include "math_utils.h"

using namespace cv;

class TrackObj
{
public:

	enum trackState
	{
		ON = 0, REQ_OFF = 1, OFF = 2, TEMP_DISABLE=3
	};

	TrackObj();
	~TrackObj();

	void predict()						{ kf->predict(); }
	void update(float newX, float newY, float dt);

	void setDebug(bool state) 			{ debug = state; }
	void setWidth(float _val) 			{ width = _val; }
	void setHeight(float _val)			{ height = _val; }
	void setOff();
	void setOn();
	void setPos(float x, float y);
	void setSize(float _w, float _h)	{ width = _w; height = _h; }
	void setState(trackState _state)	{ state = _state; }
	void setUpdtTime(double _time)		{ lastUpdtTime = _time; }
	void setUse(bool state)				{ inUse = state; }

	bool getUse()						{ return inUse; }
	float getAccel(int ind)				{ return actAccel[ind]; }
	float getDist(float newX, float newY, float rot);
	float getRandF(float min, float max);
	float getWidth() 					{ return width; }
	float getHeight() 					{ return height; }
	glm::vec2* getPos() 				{ return &actPos; }
	glm::vec2 getVel()					{ return actVel; }
	glm::vec2 getSize()					{ return glm::vec2(width, height); }
	glm::vec2 getAccumVel()				{ return velAccu; }
	glm::vec2 getMedVel()				{ return medVel; }
	glm::vec2 getAccel() 				{ return actAccel; }
	double getLastUpdtTime()			{ return lastUpdtTime; }
	double getTotalOntime()				{ return 0.001 * (double)std::chrono::duration_cast<std::chrono::milliseconds>(offDate - onDate).count(); }

	void accumVel(bool _val);
	bool isActive()						{ return (state == ON); }
	bool wasActive()					{ return (lastState == ON); }
	bool wasInactive()					{ return (lastState == OFF); }
	double onTimeDelta(double time)		{ return 0.0; }

	std::chrono::system_clock::time_point getOnDate() { return onDate; }
	std::chrono::system_clock::time_point getOffDate() { return offDate; }

	std::string getOnDateStr() {
		std::time_t t = std::chrono::system_clock::to_time_t(onDate);
		return std::ctime(&t);
	}

	std::string getOffDateStr() {
		std::time_t t = std::chrono::system_clock::to_time_t(offDate);
		return std::ctime(&t);
	}

	glm::vec4* getMoveArea()			{ return &moveArea; }
	int getUniqueID()					{ return uniqueId; }

	int				cvId;

private:
	trackState		state;
	trackState		lastState;
	bool 			debug;
	bool 			drawKalman;
	bool 			drawNumbers;
	bool 			bAccumVel;
	bool 			inUse;
	bool 			resetMed;
	int 			maxLinePoints;
	int 			index;
	std::string 	index_str;

	CvKalmanFilter* kf;

	float 			minSpeed;
	float 			speedThres;
	float 			velMed;

	glm::vec2 		actRawPos;
	float 			actRawDir;
	glm::vec2 		actPos;
	float 			actRot;
	float 			velCorrectFact;
	glm::vec2 		medVel;
	glm::vec2 		actVel;
	glm::vec2 		velAccu;
	glm::vec2 		actAccel;
	glm::vec2* 		memPos;
	int 			memPosSize;
	int 			memPosPtr;

	glm::vec4 		moveArea;

	float 			width;
	float 			height;

	double 			lastUpdtTime;
	int 			uniqueId;

	std::chrono::system_clock::time_point onDate;
	std::chrono::system_clock::time_point offDate;

};

#endif /* defined(__BlobTracking__TrackObj__) */
