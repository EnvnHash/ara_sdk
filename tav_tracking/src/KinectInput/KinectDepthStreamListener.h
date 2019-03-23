//
//  KinectDepthFrameListener.h
//  Tav_App
//
//  Created by Sven Hahne on 5/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__KinectDepthStreamListener__
#define __Tav_App__KinectDepthStreamListener__

#pragma once

#include "KinectStreamListener.h"
#ifdef HAVE_OPENCV
#include "NiTE/NISkeleton.h"
#include "NiTE/NIKinectShadow.h"
#endif
#include "FPSTimer.h"

#define MAX_DEPTH 65535

namespace tav
{

class KinectDepthStreamListener: public KinectStreamListener
{
public:
	KinectDepthStreamListener(int _nrChans);
	~KinectDepthStreamListener();
	void onNewFrame(openni::VideoStream& _stream);
	void calculateHistogram(float* pHistogram, int histogramSize,
			const openni::VideoFrameRef& frame, int normFactor);
#ifdef HAVE_OPENCV
	void addPostProc(KinectInputPostProc* _pp);
#endif
	openni::VideoFrameRef* getFrame();
	int getFrameNr();
	int getFrameNrNoHisto();
	int getMaxDepth();
	void setUpdateNis(bool _val);
	void setVMirror(bool _val);
	bool isInited();
	float* getActImgNoHisto();
	float* getActImg();
	uint8_t* getActImg8();
	uint8_t* getActImg8NoHisto();
	void lockMutex();
	void unlockMutex();
	void lockPPMutex();
	void unlockPPMutex();

private:
	openni::VideoFrameRef frameRef;

	bool init = false;
	bool updateNis = false;
	bool downloadWithHisto = true;
	bool downloadWithoutHisto = false;
	bool mode8bit = false;
	bool mode8bitNoHisto = false;
	bool mirrorV = false;
	int nrBufFrames;
	int nrChans;
	int actFramePtr = 0;
	int actFramePtrNoHisto = 0;
	int frameNr = -2;
	int frameNrNoHisto = -2;

	float pDepthHist[MAX_DEPTH];

	float** img;
	float** imgNoHisto;
	uint8_t** img8;
	uint8_t** img8NoHisto;
#ifdef HAVE_OPENCV
	NISkeleton* nis;
	std::vector<KinectInputPostProc*> postProcs;
#endif

	uint16_t depthMult;
	boost::mutex ppMtx;
	FPSTimer timer;
};

}

#endif /* defined(__Tav_App__KinectDepthFrameListener__) */
