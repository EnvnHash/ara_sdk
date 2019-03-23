/*
 * VideoTextureCvActRange.cpp
 *
 *  Created on: 19.01.2017
 *      Copyright by Sven Hahne
 */

#include "VideoTextureCvActRange.h"

namespace tav
{

VideoTextureCvActRange::VideoTextureCvActRange(ShaderCollector* _shCol, VideoTextureCv* _vidTex) :
		shCol(_shCol), vt(_vidTex)
{
	transMat = glm::mat4(1.f);
	vidRange = new VideoActivityRange(shCol, vt->getWidth(), vt->getHeight());
}

//-------------------------------------------------

void VideoTextureCvActRange::updateDt(double time, bool startStop)
{
	vt->updateDt(time, startStop);

	if (actUplTexId != vt->loadFrameToTexture())
	{
		lastTexId = actUplTexId;
		actUplTexId = vt->loadFrameToTexture();

//		vidRange->setThres( getOscPar("thres") );
//		vidRange->setTimeMedian( getOscPar("median") );
//		vidRange->setPosColThres( getOscPar("posColThres") );
//		vidRange->setHistoSmooth( getOscPar("histoSmooth") );
//		vidRange->setHistoValThres( getOscPar("valThres") );
//		vidRange->setIndValThres( getOscPar("indValThres") );

		vidRange->update(actUplTexId, lastTexId);
	}

	vidRange->updateMat();
}

//------------------------------------------------------------------------------------
/*
void VideoTextureCvActRange::update(double time, bool startStop)
{
	vt->updateDt(time, startStop);

	if (actUplTexId != vt->loadFrameToTexture())
	{
		lastTexId = actUplTexId;
		actUplTexId = vt->loadFrameToTexture();

//		vidRange->setThres( getOscPar("thres") );
//		vidRange->setTimeMedian( getOscPar("median") );
//		vidRange->setPosColThres( getOscPar("posColThres") );
//		vidRange->setHistoSmooth( getOscPar("histoSmooth") );
//		vidRange->setHistoValThres( getOscPar("valThres") );
//		vidRange->setIndValThres( getOscPar("indValThres") );

		vidRange->update(actUplTexId, lastTexId);
	}

	vidRange->updateMat();
}
*/
//------------------------------------------------------------------------------------

void VideoTextureCvActRange::calcQuadMat()
{
	transMat = *(vidRange->getInvTransMat());
	transMatFlipH = *(vidRange->getInvTransFlipHMat());
}

//------------------------------------------------------------------------------------

glm::mat4* VideoTextureCvActRange::getTransMat()
{
	calcQuadMat();
	return &transMat;
}

//------------------------------------------------------------------------------------

float* VideoTextureCvActRange::getTransMatPtr()
{
	calcQuadMat();
	return &transMat[0][0];
}

//------------------------------------------------------------------------------------

float* VideoTextureCvActRange::getTransMatFlipHPtr()
{
	calcQuadMat();
	return &transMatFlipH[0][0];
}

//------------------------------------------------------------------------------------

bool VideoTextureCvActRange::isPausing()
{
	return vt->isPausing();
}

//------------------------------------------------------------------------------------

unsigned int VideoTextureCvActRange::loadFrameToTexture()
{
	return vt->loadFrameToTexture();
}

//------------------------------------------------------------------------------------

void VideoTextureCvActRange::bindActFrame(unsigned int texUnit)
{
	vt->bindActFrame(texUnit);
}

//------------------------------------------------------------------------------------

#ifndef HAVE_CUDA
cv::Mat* VideoTextureCvActRange::getActMat()
{
	return vt->getActMat();
}
#else
cv::cuda::GpuMat* VideoTextureCvActRange::getActMat()
{
	return vt->getActMat();
}
#endif

//------------------------------------------------------------------------------------

unsigned int VideoTextureCvActRange::getWidth()
{
	return vt->getWidth();
}

//------------------------------------------------------------------------------------

unsigned int VideoTextureCvActRange::getHeight()
{
	return vt->getHeight();
}

//------------------------------------------------------------------------------------

int VideoTextureCvActRange::getActFrame()
{
	return vt->getActFrame();
}

//---------------------------------------------------------

VideoTextureCvActRange::~VideoTextureCvActRange()
{
	delete vidRange;
}

} /* namespace tav */
