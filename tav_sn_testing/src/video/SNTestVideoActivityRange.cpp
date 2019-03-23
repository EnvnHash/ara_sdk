/*
 * SNTestVideoActivityRange.cpp
 *
 *  Created on: 19.01.2017
 *      Copyright by Sven Hahne
 */

#include "SNTestVideoActivityRange.h"

#define STRINGIFY(A) #A

namespace tav
{

SNTestVideoActivityRange::SNTestVideoActivityRange(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    				SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

#ifdef HAVE_OPENCV
	VideoTextureCv** vts = static_cast<VideoTextureCv**>(scd->videoTextures);
	vt = vts[ static_cast<unsigned short>(_sceneArgs->at("vtex0")) ];

	vidRange = new VideoActivityRange(shCol, vt->getWidth(), vt->getHeight());
#endif

	addPar("thres", &thres);
	addPar("median", &median);
	addPar("posColThres", &posColThres);
	addPar("histoSmooth", &histoSmooth);
	addPar("indValThres", &indValThres);
	addPar("valThres", &valThres);
}

//----------------------------------------------------

void SNTestVideoActivityRange::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef HAVE_OPENCV
	vidRange->drawPosCol();
	//vidRange->drawHistogram();
	vidRange->drawActivityQuad();
#endif

}

//----------------------------------------------------

void SNTestVideoActivityRange::update(double time, double dt)
{

#ifdef HAVE_OPENCV
	vt->updateDt(time, false);

	if (actUplTexId != vt->loadFrameToTexture())
	{
		lastTexId = actUplTexId;
		actUplTexId = vt->loadFrameToTexture();

		vidRange->setThres( thres );
		vidRange->setTimeMedian( median );
		vidRange->setPosColThres( posColThres );
		vidRange->setHistoSmooth( histoSmooth );
		vidRange->setHistoValThres( valThres );
		vidRange->setIndValThres( indValThres );

		vidRange->update(actUplTexId, lastTexId);
	}
#endif

}

//----------------------------------------------------

SNTestVideoActivityRange::~SNTestVideoActivityRange()
{
}

}
/* namespace tav */
