//
//  KinectStreamListener.cpp
//  Tav_App
//
//  Created by Sven Hahne on 24/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "KinectStreamListener.h"

namespace tav
{

KinectStreamListener::KinectStreamListener() :
		openni::VideoStream::NewFrameListener()
{
	mtx = new boost::mutex();
}

//------------------------------------------

KinectStreamListener::~KinectStreamListener()
{
	delete mtx;
}

//------------------------------------------

boost::mutex* KinectStreamListener::getMtx()
{
	return mtx;
}

}
