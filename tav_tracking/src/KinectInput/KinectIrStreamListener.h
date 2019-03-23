//
//  KinectIrStreamListener.h
//  Tav_App
//
//  Created by Sven Hahne on 25/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__KinectIrStreamListener__
#define __Tav_App__KinectIrStreamListener__

#pragma once

#include "KinectStreamListener.h"

namespace tav
{

typedef struct
{
	float** pHist;
	unsigned short nrBuffers = 5;
	unsigned short bufPtr = 0;
	const int maxIrBright = 10000;
} HistoPar;

class KinectIrStreamListener: public KinectStreamListener
{
public:
	KinectIrStreamListener(HistoPar* _pHistPar, float _amp = 1.f);
	~KinectIrStreamListener();
	void onNewFrame(openni::VideoStream& _stream);
	openni::VideoFrameRef* getFrame();
	int getFrameNr();
	int getFrameNrNoHisto()
	{
		return -1;
	}
	bool isInited();
	uint8_t* getActImg();
	void calculateNorm(const openni::VideoFrameRef& frame, int histogramSize);
	void rot90();
	void lockMutex();
	void unlockMutex();

private:
	openni::VideoFrameRef frameRef;

	bool init = false;
	bool rotateNinety;
	bool useHisto;

	int nrBufFrames;
	int actFramePtr = 0;

	float* normFact;
	float normMed = 10.f;
	float amp;
	HistoPar* pHistPar;

	static const int maxIrBright = 10000;
	float pDepthHist[maxIrBright];

	uint8_t** img;
};

}

#endif /* defined(__Tav_App__KinectDepthFrameListener__) */
