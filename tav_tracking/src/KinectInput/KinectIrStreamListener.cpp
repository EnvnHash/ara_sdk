//
//  KinectIrStreamListener.cpp
//  Tav_App
//
//  Created by Sven Hahne on 25/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "KinectIrStreamListener.h"

namespace tav
{

KinectIrStreamListener::KinectIrStreamListener(HistoPar* _pHistPar, float _amp) :
		KinectStreamListener(), nrBufFrames(7), useHisto(true), pHistPar(
				_pHistPar), amp(_amp), rotateNinety(false)
{
}

//------------------------------------------

void KinectIrStreamListener::onNewFrame(openni::VideoStream& _stream)
{
	const openni::VideoMode& vMode = _stream.getVideoMode();

	// init
	if (!init)
	{
		if (_stream.isValid())
		{
			const openni::VideoMode& vMode = _stream.getVideoMode();

			img = new uint8_t*[nrBufFrames];
			for (int i = 0; i < nrBufFrames; i++)
			{
				img[i] = new uint8_t[vMode.getResolutionX()
						* vMode.getResolutionY()];
				memset(img[i], 0,
						vMode.getResolutionX() * vMode.getResolutionY()
								* sizeof(uint8_t));
			}

		}
		else
		{
			printf("color stream not valid...\n");
		}

		init = true;
	}

	// read new frame
	_stream.readFrame(&frameRef);

	// download frameRef
	if (frameRef.isValid())
	{
		// wird benötigt, scheinbar werden nicht alle pixel neu gesetzt
		memset(img[actFramePtr], 0,
				vMode.getResolutionX() * vMode.getResolutionY()
						* sizeof(uint8_t));

		// used to normalize the data
		if (useHisto)
			calculateNorm(frameRef, maxIrBright);

		//unsigned short fixHistPtr = pHistPar->bufPtr;

		// get pointer to frame data
		const openni::DepthPixel* pDepthRow =
				(const openni::DepthPixel*) frameRef.getData();

		// get pointer to destination data
		uint8_t* pTexRow = img[actFramePtr]
				+ frameRef.getCropOriginY() * vMode.getResolutionX();

		// get the size of one row in the depth data
		int rowSize = frameRef.getStrideInBytes() / sizeof(openni::DepthPixel);

		// run through the depth data
		if (useHisto && !rotateNinety)
		{
			for (int y = 0; y < frameRef.getHeight(); ++y)
			{
				const openni::DepthPixel* pDepth = pDepthRow;
				uint8_t* pTex = pTexRow + frameRef.getCropOriginX();

				for (int x = 0; x < frameRef.getWidth(); ++x, ++pDepth, ++pTex)
					if (*pDepth != 0)
						*pTex = pDepthHist[*pDepth];

				pDepthRow += rowSize;
				pTexRow += vMode.getResolutionX();
			}
		}
		else if (useHisto && rotateNinety)
		{
			const openni::DepthPixel* pDepth;
			uint8_t* pTex;

			for (int y = 0; y < frameRef.getWidth(); ++y)
			{
				for (int x = 0; x < frameRef.getHeight(); ++x)
				{
					pTex = img[actFramePtr] + y * frameRef.getHeight() + x;
					pDepth = pDepthRow + x * frameRef.getWidth()
							+ (frameRef.getWidth() - y - 1);

					*pTex = pDepthHist[*pDepth];
				}
			}

		}
		else if (!useHisto && rotateNinety)
		{
			const openni::DepthPixel* pDepth;
			uint8_t* pTex;

			for (int y = 0; y < frameRef.getWidth(); ++y)
			{
				for (int x = 0; x < frameRef.getHeight(); ++x)
				{
					pTex = img[actFramePtr] + y * frameRef.getHeight() + x;
					pDepth = pDepthRow + x * frameRef.getWidth()
							+ (frameRef.getWidth() - y - 1);

					*pTex = *pDepth / 4;
				}
			}

		}
		else
		{
			for (int y = 0; y < frameRef.getHeight(); ++y)
			{
				const openni::DepthPixel* pDepth = pDepthRow;
				uint8_t* pTex = pTexRow + frameRef.getCropOriginX();

				for (int x = 0; x < frameRef.getWidth(); ++x, ++pDepth, ++pTex)
					if (*pDepth != 0)
						*pTex = (int) std::min(*pDepth / 4.f * amp, 255.f);

				pDepthRow += rowSize;
				pTexRow += vMode.getResolutionX();
			}
		}
	}

	frameNr++;
	actFramePtr++;
	if (actFramePtr >= nrBufFrames)
		actFramePtr = 0;
}

//------------------------------------------

void KinectIrStreamListener::calculateNorm(const openni::VideoFrameRef& frame,
		int histogramSize)
{
	//unsigned short ptrPlusOne = (pHistPar->bufPtr +1) % pHistPar->nrBuffers;
	const openni::DepthPixel* pDepth =
			(const openni::DepthPixel*) frame.getData();

//    memset(pHistPar->pHist[ptrPlusOne], 0, histogramSize *sizeof(float));
	memset(pDepthHist, 0, histogramSize * sizeof(float));

	// Gives the length of one row of pixels in pixels - the width of the frame
	// = if the frame is cropped the size of the crop area
	// if no cropping this is zero
	int restOfRow = frame.getStrideInBytes() / sizeof(openni::DepthPixel)
			- frame.getWidth();
	int height = frame.getHeight();
	int width = frame.getWidth();

	// go through the whole depth data from the device
	// count the number of pixels processed
	unsigned int nNumberOfPoints = 0;
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x, ++pDepth)
		{
			if (*pDepth != 0 && *pDepth < histogramSize)
			{
				pDepthHist[*pDepth]++;
				nNumberOfPoints++;
			}
		}

		pDepth += restOfRow;
	}

	// sum all histogram values
	// add the value at [0] to [1]
	// add the value at [1] to [2]
	for (int nIndex = 1; nIndex < histogramSize; nIndex++)
		pDepthHist[nIndex] += pDepthHist[nIndex - 1];

	// when there was data processed
	// run through the whole histogram
	// normalize
	if (nNumberOfPoints)
		for (int nIndex = 1; nIndex < histogramSize; nIndex++)
			pDepthHist[nIndex] = 255.f * (pDepthHist[nIndex] / nNumberOfPoints);

	//pHistPar->bufPtr = ptrPlusOne;
}

//------------------------------------------

openni::VideoFrameRef* KinectIrStreamListener::getFrame()
{
	return &frameRef;
}

//------------------------------------------

int KinectIrStreamListener::getFrameNr()
{
	return frameNr;
}

//------------------------------------------

bool KinectIrStreamListener::isInited()
{
	return init;
}

//------------------------------------------

uint8_t* KinectIrStreamListener::getActImg()
{
	return img[(actFramePtr - 1 + nrBufFrames) % nrBufFrames];
}

//------------------------------------------

void KinectIrStreamListener::rot90()
{
	rotateNinety = true;
}

//------------------------------------------

void KinectIrStreamListener::lockMutex()
{
}

//------------------------------------------

void KinectIrStreamListener::unlockMutex()
{
}

//------------------------------------------

KinectIrStreamListener::~KinectIrStreamListener()
{
}

}
