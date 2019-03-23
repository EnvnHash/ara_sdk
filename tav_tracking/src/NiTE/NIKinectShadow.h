//
//  NIKinectShadowCb.h
//  Tav_App
//
//  Created by Sven Hahne on 10/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__NIKinectShadowCb__
#define __Tav_App__NIKinectShadowCb__

#pragma once

#include <cstring>
#include <cstdlib>
#include <map>
#include <inttypes.h>
#include <stdio.h>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <glm/gtx/string_cast.hpp>

#include <NiTE.h>

#include "KinectInput/KinectInputPostProc.h"
#include <FPSTimer.h>
#include <math_utils.h>

namespace tav
{

class NIKinectShadow: public KinectInputPostProc
{
public:
	NIKinectShadow(int _width, int _height, int _nrChans, kinectMapping* _kMap,
			nite::UserTrackerFrameRef* _userTrackerFrame);
	~NIKinectShadow();
	void update(openni::VideoStream& _stream, openni::VideoFrameRef* _frame);
	bool isReady();
	virtual void processQueue(unsigned int N, openni::VideoStream* depthStream,
			openni::VideoFrameRef* depthFrame, const nite::UserMap& userLabels);
	uint8_t* getShadowImg(int frameNr);
	int getFrameNr();
	int getBufPtr();
	float getRandF(float min, float max);
	glm::vec3* getUserCenter(short userNr);

private:
	bool 			useThreads;
	bool 			ready = false;
	bool 			threadRunning = false;

	int 			nrThreads;

	float 			cropLeft = 0;
	float 			cropRight = 0;
	float 			cropDepth = 0;

	int 			frameNr = -1;
	int 			width;
	int 			height;
	int 			nrRows;
	int 			colRowSize;
	int 			nrChans;
	unsigned int 	fpsInt;
	unsigned int 	fpsCnt;

	uint8_t** 		shadow_pp;
	int 			shadow_pp_ptr;
	int* 			shadow_pp_frame_map;
	int 			nrShadowBufs;
	int 			maxLineMult;

	float 			shadow_tBlend;

	boost::thread** threads;

	nite::UserTrackerFrameRef* userTrackerFrame = 0;
	kinectMapping*	kMap;
	FPSTimer		fpsTimer;
	std::vector<glm::vec3> userCenters;
};

}

#endif /* defined(__Tav_App__NIKinectShadowCb__) */
