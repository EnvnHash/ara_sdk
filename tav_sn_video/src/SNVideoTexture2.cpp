//
// SNVideoTexture2.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNVideoTexture2.h"

namespace tav
{

SNVideoTexture2::SNVideoTexture2(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	osc = static_cast<OSCData*>(scd->osc);

#ifdef HAVE_OPENCV
	VideoTextureCv** vts = static_cast<VideoTextureCv**>(scd->videoTextures);
	vt = static_cast<VideoTextureCv*>(vts[0]);
	// vt->set_frame(0, true);
#endif

	quad = new QuadArray(35, 35, -1.0f, -1.0f, 2.f, 2.f, 1.f, 1.f, 1.f, 1.f);
	quad->rotate(M_PI, 1.f, 0.f, 0.f);
}

//------------------------------------------

void SNVideoTexture2::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	sendStdShaderInit(_shader);

#ifdef HAVE_OPENCV
	vt->updateDt(dt * scd->sceneSpeed * osc->videoSpeed,
			static_cast<bool>(osc->startStopVideo));
	vt->loadFrameToTexture(time);

	useTextureUnitInd(0, vt->getTex(), _shader, _tfo);
#endif

	quad->draw(_tfo);
}

//------------------------------------------

void SNVideoTexture2::update(double time, double dt)
{
}

//------------------------------------------

SNVideoTexture2::~SNVideoTexture2()
{
	delete quad;
}
}
