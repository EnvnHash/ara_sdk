//
//  KinectDepthFrameListener.h
//  Tav_App
//
//  Created by Sven Hahne on 5/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__KinectColorStreamListener__
#define __Tav_App__KinectColorStreamListener__

#pragma once

#include <KinectInput/KinectStreamListener.h>

namespace tav
{

class KinectColorStreamListener: public KinectStreamListener
{
public:
	KinectColorStreamListener(int _nrChans);
	~KinectColorStreamListener();
	void onNewFrame(openni::VideoStream& _stream);
	openni::VideoFrameRef* getFrame();
	int getFrameNr();
	int getFrameNrNoHisto()
	{
		return -1;
	}
	void setVMirror(bool _val);
	bool isInited();
	uint8_t* getActImg();
	uint8_t* getImg(int offset);
	uint8_t* getActGrayImg();
	void useGray(bool _val);
	void rot90();
	void lockMutex();
	void unlockMutex();

private:
	openni::VideoFrameRef frameRef;

	bool init = false;
	bool bGray = false;
	bool rotateNinety;
	bool mirrorV = false;

	int nrBufFrames;
	int nrChans;
	int actFramePtr = 0;

	uint8_t** img;
	uint8_t** grayImg;
};

}
#endif /* defined(__Tav_App__KinectDepthFrameListener__) */
