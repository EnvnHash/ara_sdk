//
//  KinectStreamListener.h
//  Tav_App
//
//  Created by Sven Hahne on 24/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef Tav_App_KinectStreamListener_h
#define Tav_App_KinectStreamListener_h

#pragma once

#include <iostream>
#include <stdio.h>
#include <boost/thread/mutex.hpp>
#include <OpenNI.h>

#include <headers/gl_header.h>

namespace tav
{

class KinectStreamListener: public openni::VideoStream::NewFrameListener
{
public:
	KinectStreamListener();
	~KinectStreamListener();
	virtual void onNewFrame(openni::VideoStream& _stream) =0;
	virtual openni::VideoFrameRef* getFrame() =0;
	virtual int getFrameNr() =0;
	virtual int getFrameNrNoHisto() =0;
	virtual bool isInited() =0;
	virtual boost::mutex* getMtx();
	virtual void lockMutex() =0;
	virtual void unlockMutex() =0;

	int frameNr = -1;
	boost::mutex* mtx;
};

}

#endif /* defined(__Tav_App__KinectDepthFrameListener__) */
