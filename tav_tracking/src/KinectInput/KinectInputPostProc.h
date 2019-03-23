//
//  KinectInputCallback.h
//  Tav_App
//
//  Created by Sven Hahne on 10/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef Tav_App_KinectInputCallback_h
#define Tav_App_KinectInputCallback_h

#pragma once 

#include <OpenNI.h>
#include <glm/glm.hpp>

namespace tav
{

typedef struct
{
	glm::vec2* offset;
	glm::vec2* scale;
	glm::vec2* distScale;
} kinectMapping;

class KinectInputPostProc
{
public:
	KinectInputPostProc() :
			doUpdate(false)
	{}

	~KinectInputPostProc()
	{}

	virtual void update(openni::VideoStream& _stream,
			openni::VideoFrameRef* _frame)=0;
	virtual bool isReady()=0;
	bool doUpdate;
};

}

#endif
