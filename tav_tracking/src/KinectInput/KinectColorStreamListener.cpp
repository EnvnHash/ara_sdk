//
//  KinectDepthFrameListener.cpp
//  Tav_App
//
//  Created by Sven Hahne on 5/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//
// Event Based Frame Reading
//

#include "KinectColorStreamListener.h"

namespace tav
{

KinectColorStreamListener::KinectColorStreamListener(int _nrChans) :
		KinectStreamListener(), nrBufFrames(4), nrChans(_nrChans), rotateNinety(
				false)
{
}

//---------------------------------------------------------------

KinectColorStreamListener::~KinectColorStreamListener()
{
}

//---------------------------------------------------------------

void KinectColorStreamListener::onNewFrame(openni::VideoStream& _stream)
{
	if (_stream.isValid())
	{
		mtx->lock();

		// init
		if (!init)
		{
			const openni::VideoMode& vMode = _stream.getVideoMode();

			img = new uint8_t*[nrBufFrames];
			grayImg = new uint8_t*[nrBufFrames];
			for (int i = 0; i < nrBufFrames; i++)
			{
				img[i] = new uint8_t[vMode.getResolutionX()
						* vMode.getResolutionY() * nrChans];
				grayImg[i] = new uint8_t[vMode.getResolutionX()
						* vMode.getResolutionY()];
				memset(img[i], 0,
						vMode.getResolutionX() * vMode.getResolutionY()
								* sizeof(uint8_t) * nrChans);
				memset(grayImg[i], 0,
						vMode.getResolutionX() * vMode.getResolutionY()
								* sizeof(uint8_t));
			}

			init = true;
		}

		// read new frame
		_stream.readFrame(&frameRef);

		if (frameRef.isValid())
		{
			if (!bGray && !rotateNinety && !mirrorV)
			{
				memcpy(img[actFramePtr], frameRef.getData(),
						frameRef.getHeight() * frameRef.getWidth() * nrChans
								* sizeof(uint8_t));

			}
			else if (!bGray && !rotateNinety && mirrorV)
			{
				const openni::RGB888Pixel* pImageRow =
						(const openni::RGB888Pixel*) frameRef.getData();
				int rowSize = frameRef.getStrideInBytes()
						/ sizeof(openni::RGB888Pixel);
				uint8_t* pTexRow = img[actFramePtr]
						+ (frameRef.getHeight() - 1) * frameRef.getWidth()
								* nrChans;

				for (int y = 0; y < frameRef.getHeight(); ++y)
				{
					const openni::RGB888Pixel* pImage = pImageRow;
					uint8_t* pTex = pTexRow
							+ frameRef.getCropOriginX() * nrChans;

					for (int x = 0; x < frameRef.getWidth(); ++x, ++pImage)
					{
						*pTex = (*pImage).r;
						pTex++;
						*pTex = (*pImage).g;
						pTex++;
						*pTex = (*pImage).b;
						pTex++;
					}

					pImageRow += rowSize;
					pTexRow -= frameRef.getWidth() * nrChans;
				}

			}
			else if (bGray && !rotateNinety)
			{
				const openni::RGB888Pixel* pImageRow =
						(const openni::RGB888Pixel*) frameRef.getData();
				int rowSize = frameRef.getStrideInBytes()
						/ sizeof(openni::RGB888Pixel);

				uint8_t* pTexRow = grayImg[actFramePtr]
						+ frameRef.getCropOriginY() * frameRef.getWidth();
				if (mirrorV)
					pTexRow = grayImg[actFramePtr]
							+ (frameRef.getHeight() - 1) * frameRef.getWidth();

				for (int y = 0; y < frameRef.getHeight(); ++y)
				{
					const openni::RGB888Pixel* pImage = pImageRow;
					uint8_t* pTex = pTexRow + frameRef.getCropOriginX();

					for (int x = 0; x < frameRef.getWidth();
							++x, ++pImage, ++pTex)
					{
						*pTex = (int) ((float) ((*pImage).r + (*pImage).g
								+ (*pImage).b) * 0.3333f);
					}

					pImageRow += rowSize;
					if (!mirrorV)
						pTexRow += frameRef.getWidth();
					else
						pTexRow -= frameRef.getWidth();
				}

			}
			else if (!bGray && rotateNinety)
			{
				const openni::RGB888Pixel* pImageRow =
						(const openni::RGB888Pixel*) frameRef.getData();
				int rowSize = frameRef.getStrideInBytes()
						/ sizeof(openni::RGB888Pixel);

				uint8_t* pTexRow = img[actFramePtr]
						+ frameRef.getCropOriginY() * frameRef.getWidth()
								* nrChans;
				const openni::RGB888Pixel* pImage;
				uint8_t* pTex;

				for (int y = 0; y < frameRef.getWidth(); ++y)
				{
					for (int x = 0; x < frameRef.getHeight();
							++x, ++pImage, ++pTex)
					{
						pTex = img[actFramePtr]
								+ ((y * frameRef.getHeight() + x) * nrChans);
						pImage = pImageRow + x * frameRef.getWidth()
								+ (frameRef.getWidth() - y - 1);

						*pTex = (*pImage).r;
						pTex++;
						*pTex = (*pImage).g;
						pTex++;
						*pTex = (*pImage).b;
						pTex++;
					}
				}
			}
			else
			{
				const openni::RGB888Pixel* pImageRow =
						(const openni::RGB888Pixel*) frameRef.getData();
				int rowSize = frameRef.getStrideInBytes()
						/ sizeof(openni::RGB888Pixel);

				uint8_t* pTexRow = grayImg[actFramePtr]
						+ frameRef.getCropOriginY() * frameRef.getWidth();
				if (mirrorV)
					pTexRow = grayImg[actFramePtr]
							+ (frameRef.getHeight() - 1) * frameRef.getWidth();

				const openni::RGB888Pixel* pImage;
				uint8_t* pTex;

				for (int y = 0; y < frameRef.getWidth(); ++y)
				{
					for (int x = 0; x < frameRef.getHeight();
							++x, ++pImage, ++pTex)
					{
						pTex = grayImg[actFramePtr] + y * frameRef.getHeight()
								+ x;
						pImage = pImageRow + x * frameRef.getWidth()
								+ (frameRef.getWidth() - y - 1);

						*pTex = (int) ((float) ((*pImage).r + (*pImage).g
								+ (*pImage).b) * 0.3333f);
					}
				}
			}

			frameNr++;

			//  printf("frameNr: %d\n", frameNr);

			actFramePtr++;
			if (actFramePtr >= nrBufFrames)
				actFramePtr = 0;

		}
		else
		{
			printf("KinectColorStreamListener:: frameRef not valid...\n");
		}

		mtx->unlock();

	}
	else
	{
		printf("KinectColorStreamListener:: color stream not valid...\n");
	}
}

//---------------------------------------------------------------

openni::VideoFrameRef* KinectColorStreamListener::getFrame()
{
	return &frameRef;
}

//---------------------------------------------------------------

int KinectColorStreamListener::getFrameNr()
{
	return frameNr;
}

//---------------------------------------------------------------

bool KinectColorStreamListener::isInited()
{
	return init;
}

//---------------------------------------------------------------

uint8_t* KinectColorStreamListener::getActGrayImg()
{
	return grayImg[(actFramePtr - 1 + nrBufFrames) % nrBufFrames];
}

//---------------------------------------------------------------

uint8_t* KinectColorStreamListener::getActImg()
{
	return img[(actFramePtr - 1 + nrBufFrames) % nrBufFrames];
}

//---------------------------------------------------------------

uint8_t* KinectColorStreamListener::getImg(int offset)
{
	return img[(actFramePtr - 1 + offset + nrBufFrames) % nrBufFrames];
}

//---------------------------------------------------------------

void KinectColorStreamListener::setVMirror(bool _val)
{
	mirrorV = _val;
}

//---------------------------------------------------------------

void KinectColorStreamListener::useGray(bool _val)
{
	bGray = _val;
}

//---------------------------------------------------------------

void KinectColorStreamListener::rot90()
{
	rotateNinety = true;
}

//---------------------------------------------------------------

void KinectColorStreamListener::lockMutex()
{
	mtx->lock();
}

//---------------------------------------------------------------

void KinectColorStreamListener::unlockMutex()
{
	mtx->unlock();
}

}
