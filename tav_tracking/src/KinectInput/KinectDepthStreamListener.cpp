//
//  KinectDepthFrameListener.cpp
//  Tav_App
//
//  Created by Sven Hahne on 5/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//
//  Event Based Frame Reading
//
//  mit Histogram können die were variiern,
//  ohne histogram müssen die werte angehoben werden,
//  sonst sieht man nichts
//

#include "KinectDepthStreamListener.h"

namespace tav
{

KinectDepthStreamListener::KinectDepthStreamListener(int _nrChans) :
		KinectStreamListener(), nrBufFrames(8), nrChans(_nrChans), depthMult(18)
{
	timer.showFps(true);
}

//---------------------------------------------------------------

KinectDepthStreamListener::~KinectDepthStreamListener()
{
}

//---------------------------------------------------------------

void KinectDepthStreamListener::onNewFrame(openni::VideoStream& _stream)
{
	if (_stream.isValid())
	{
		mtx->lock();

		const openni::VideoMode& vMode = _stream.getVideoMode();

		// init
		if (!init)
		{
			// bitmap init
			img = new float*[nrBufFrames];
			imgNoHisto = new float*[nrBufFrames];
			img8 = new uint8_t*[nrBufFrames];
			img8NoHisto = new uint8_t*[nrBufFrames];

			for (int i = 0; i < nrBufFrames; i++)
			{
				img[i] = new float[vMode.getResolutionX()
						* vMode.getResolutionY() * nrChans];
				memset(img[i], 0,
						vMode.getResolutionX() * vMode.getResolutionY()
								* sizeof(float) * nrChans);

				imgNoHisto[i] = new float[vMode.getResolutionX()
						* vMode.getResolutionY() * nrChans];
				memset(imgNoHisto[i], 0,
						vMode.getResolutionX() * vMode.getResolutionY()
								* sizeof(float) * nrChans);

				img8[i] = new uint8_t[vMode.getResolutionX()
						* vMode.getResolutionY() * nrChans];
				memset(img8[i], 0,
						vMode.getResolutionX() * vMode.getResolutionY()
								* sizeof(uint8_t) * nrChans);

				img8NoHisto[i] = new uint8_t[vMode.getResolutionX()
						* vMode.getResolutionY() * nrChans];
				memset(img8NoHisto[i], 0,
						vMode.getResolutionX() * vMode.getResolutionY()
								* sizeof(uint8_t) * nrChans);
			}
		}

		init = true;

		// read new frame
		_stream.readFrame(&frameRef);

		if (frameRef.isValid())
		{

			// download frameRef
			// wird benötigt, scheinbar werden nicht alle pixel neu gesetzt
			if (downloadWithHisto)
			{
				// get pointer to frame data
				const openni::DepthPixel* pDepthRow =
						(const openni::DepthPixel*) frameRef.getData();

				// get the size of one row in the depth data
				int rowSize = frameRef.getStrideInBytes()
						/ sizeof(openni::DepthPixel);

				if (!mode8bit)
				{
					float* pTexRow;

					memset(img[actFramePtr], 0,
							vMode.getResolutionX() * vMode.getResolutionY()
									* sizeof(float) * nrChans);
					calculateHistogram(pDepthHist, MAX_DEPTH, frameRef, 65536);

					if (mirrorV)
						pTexRow = img[actFramePtr]
								+ (vMode.getResolutionY() - 1)
										* vMode.getResolutionX();
					else
						pTexRow = img[actFramePtr]
								+ frameRef.getCropOriginY()
										* vMode.getResolutionX();

					for (int y = 0; y < frameRef.getHeight(); ++y)
					{
						const openni::DepthPixel* pDepth = pDepthRow;
						float* pTex = pTexRow + frameRef.getCropOriginX();
						for (int x = 0; x < frameRef.getWidth();
								++x, ++pDepth, ++pTex)
							if (*pDepth != 0)
							{
								*pTex = float(pDepthHist[*pDepth]);
								//std::cout << *pTex << std::endl;
							}

						pDepthRow += rowSize;
						if (!mirrorV)
							pTexRow += vMode.getResolutionX();
						else
							pTexRow -= vMode.getResolutionX();
					}
				}
				else
				{
					uint8_t* pTexRow8;

					memset(img8[actFramePtr], 0,
							vMode.getResolutionX() * vMode.getResolutionY()
									* sizeof(uint8_t) * nrChans);
					calculateHistogram(pDepthHist, MAX_DEPTH, frameRef, 255);

					if (mirrorV)
						pTexRow8 = img8[actFramePtr]
								+ (vMode.getResolutionY() - 1)
										* vMode.getResolutionX();
					else
						pTexRow8 = img8[actFramePtr]
								+ frameRef.getCropOriginY()
										* vMode.getResolutionX();

					for (int y = 0; y < frameRef.getHeight(); ++y)
					{
						const openni::DepthPixel* pDepth = pDepthRow;
						uint8_t* pTex8 = pTexRow8 + frameRef.getCropOriginX();
						for (int x = 0; x < frameRef.getWidth();
								++x, ++pDepth, ++pTex8)
							if (*pDepth != 0)
								*pTex8 = pDepthHist[*pDepth];

						pDepthRow += rowSize;
						if (!mirrorV)
							pTexRow8 += vMode.getResolutionX();
						else
							pTexRow8 -= vMode.getResolutionX();
					}
				}

				frameNr++;
				actFramePtr = (actFramePtr + 1) % nrBufFrames;
			}

			if (downloadWithoutHisto)
			{
				// get pointer to frame data
				const openni::DepthPixel* pDepthRow =
						(const openni::DepthPixel*) frameRef.getData();

				// get the size of one row in the depth data
				int rowSize = frameRef.getStrideInBytes()
						/ sizeof(openni::DepthPixel);

				if (!mode8bitNoHisto)
				{
					float* pTexRow;

					memset(imgNoHisto[actFramePtrNoHisto], 0,
							vMode.getResolutionX() * vMode.getResolutionY()
									* sizeof(float) * nrChans);

					// bei vertikaler spiegelung pointer auf die unterste Zeile setzen
					if (mirrorV)
						pTexRow = imgNoHisto[actFramePtrNoHisto]
								+ (vMode.getResolutionY() - 1)
										* vMode.getResolutionX();
					else
						pTexRow = imgNoHisto[actFramePtrNoHisto]
								+ frameRef.getCropOriginY()
										* vMode.getResolutionX();

					for (int y = 0; y < frameRef.getHeight(); ++y)
					{
						const openni::DepthPixel* pDepth = pDepthRow;
						float* pTex = pTexRow + frameRef.getCropOriginX();

						for (int x = 0; x < frameRef.getWidth();
								++x, ++pDepth, ++pTex)
							if (*pDepth != 0)
								*pTex = float(*pDepth);

						pDepthRow += rowSize;

						if (!mirrorV)
							pTexRow += vMode.getResolutionX();
						else
							pTexRow -= vMode.getResolutionX();
					}
				}
				else
				{
					uint8_t* pTexRow8;

					memset(img8NoHisto[actFramePtrNoHisto], 0,
							vMode.getResolutionX() * vMode.getResolutionY()
									* sizeof(uint8_t) * nrChans);

					if (mirrorV)
						pTexRow8 = img8NoHisto[actFramePtrNoHisto]
								+ (vMode.getResolutionY() - 1)
										* vMode.getResolutionX();
					else
						pTexRow8 = img8NoHisto[actFramePtrNoHisto]
								+ frameRef.getCropOriginY()
										* vMode.getResolutionX();

					for (int y = 0; y < frameRef.getHeight(); ++y)
					{
						const openni::DepthPixel* pDepth = pDepthRow;
						uint8_t* pTex8 = pTexRow8 + frameRef.getCropOriginX();

						for (int x = 0; x < frameRef.getWidth();
								++x, ++pDepth, ++pTex8)
							if (*pDepth != 0)
								*pTex8 =
										static_cast<int>(static_cast<float>(*pDepth)
												* 0.00390625f); // / 255. könnte aber auch weniger

						pDepthRow += rowSize;
						if (!mirrorV)
							pTexRow8 += vMode.getResolutionX();
						else
							pTexRow8 -= vMode.getResolutionX();
					}
				}

				frameNrNoHisto++;
				actFramePtrNoHisto = (actFramePtrNoHisto + 1) % nrBufFrames;
			}

			// post processing one thread for each process
			for (std::vector<KinectInputPostProc*>::iterator it =
					postProcs.begin(); it != postProcs.end(); ++it)
				if ((*it)->isReady())
					(*it)->update(_stream, &frameRef);

		}
		else
		{
			printf("depth frameRef not valid...\n");
		}

		mtx->unlock();

	}
	else
	{
		printf("depth stream not valid...\n");
	}
}

//---------------------------------------------------------------

void KinectDepthStreamListener::calculateHistogram(float* pHistogram,
		int histogramSize, const openni::VideoFrameRef& frame, int normFactor)
{
	// get a pointer to the actual depth data
	const openni::DepthPixel* pDepth =
			(const openni::DepthPixel*) frame.getData();

	// reset all histogram data
	// each index of the histogram corresponds a depth value
	memset(pHistogram, 0, histogramSize * sizeof(float));

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
			if (*pDepth != 0)
			{
				// if the depth value at this position is not black
				// take the depth value as an index and add one to the value at this index
				// e.g. if there are a lot absolut bright values, the value at the index of
				// this brightness value will be high
				pHistogram[*pDepth]++;
				nNumberOfPoints++;
			}
		}
		pDepth += restOfRow;
	}

	// sum all histogram values
	// add the value at [0] to [1]
	// add the summed value at [1] to [2]
	for (int nIndex = 1; nIndex < histogramSize; nIndex++)
		pHistogram[nIndex] += pHistogram[nIndex - 1];

	// when there was data processed
	// run through the whole histogram
	// normalize
	if (nNumberOfPoints)
		for (int nIndex = 1; nIndex < histogramSize; nIndex++)
			pHistogram[nIndex] =
					(1.0f - (pHistogram[nIndex] / nNumberOfPoints));
}

//---------------------------------------------------------------

#ifdef HAVE_OPENCV
void KinectDepthStreamListener::addPostProc(KinectInputPostProc* _pp)
{
	ppMtx.lock();
	postProcs.push_back(_pp);
	ppMtx.unlock();
}
#endif

//---------------------------------------------------------------

openni::VideoFrameRef* KinectDepthStreamListener::getFrame()
{
	return &frameRef;
}

//---------------------------------------------------------------

int KinectDepthStreamListener::getMaxDepth()
{
	return MAX_DEPTH;
}

//---------------------------------------------------------------

int KinectDepthStreamListener::getFrameNr()
{
	return frameNr;
}

//---------------------------------------------------------------

int KinectDepthStreamListener::getFrameNrNoHisto()
{
	return frameNrNoHisto;
}

//---------------------------------------------------------------

void KinectDepthStreamListener::setUpdateNis(bool _val)
{
	updateNis = _val;
}

//---------------------------------------------------------------

void KinectDepthStreamListener::setVMirror(bool _val)
{
	mirrorV = _val;
}

//---------------------------------------------------------------

bool KinectDepthStreamListener::isInited()
{
	return init;
}

//---------------------------------------------------------------

float* KinectDepthStreamListener::getActImgNoHisto()
{
	float* out = 0;

	downloadWithoutHisto = true;
	mode8bitNoHisto = false;
	if (init)
		out = imgNoHisto[(actFramePtrNoHisto - 1 + nrBufFrames) % nrBufFrames];

	return out;
}

//---------------------------------------------------------------

float* KinectDepthStreamListener::getActImg()
{
	float* out = 0;
	downloadWithHisto = true;
	mode8bit = false;

	if (init)
		out = img[(actFramePtr - 1 + nrBufFrames) % nrBufFrames];

	return out;
}

//---------------------------------------------------------------

uint8_t* KinectDepthStreamListener::getActImg8NoHisto()
{
	uint8_t* out = 0;
	downloadWithoutHisto = true;
	mode8bitNoHisto = true;
	if (init)
		out = img8NoHisto[(actFramePtrNoHisto - 1 + nrBufFrames) % nrBufFrames];

	return out;
}

//---------------------------------------------------------------

uint8_t* KinectDepthStreamListener::getActImg8()
{
	uint8_t* out = 0;
	downloadWithHisto = true;
	mode8bit = true;
	if (init)
		out = img8[(actFramePtr - 1 + nrBufFrames) % nrBufFrames];

	return out;
}

//---------------------------------------------------------------

void KinectDepthStreamListener::lockMutex()
{
	mtx->lock();
}

//---------------------------------------------------------------

void KinectDepthStreamListener::unlockMutex()
{
	mtx->unlock();
}

//---------------------------------------------------------------

void KinectDepthStreamListener::lockPPMutex()
{
	ppMtx.lock();
}

//---------------------------------------------------------------

void KinectDepthStreamListener::unlockPPMutex()
{
	ppMtx.unlock();
}

}
