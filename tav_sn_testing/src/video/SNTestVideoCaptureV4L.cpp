//
// SNTestVideoCaptureV4L.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestVideoCaptureV4L.h"

namespace tav
{

SNTestVideoCaptureV4L::SNTestVideoCaptureV4L(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    				SceneNode(_scd, _sceneArgs,"NoLight")
{
	vt = new V4L((ShaderCollector*) _scd->shaderCollector, (char*) "/dev/video0",
			encodingmethod_e::YUV422, 1280, 720);

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f,
			NULL, 1, true);

	// sample controls
	addPar("Brightness", &brightness);
	addPar("Gain", &gain);
	addPar("Saturation", &saturation);
	addPar("Hue", &hue);
}

//----------------------------------------------------

void SNTestVideoCaptureV4L::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_CULL_FACE);
	sendStdShaderInit(_shader);

	useTextureUnitInd(0, vt->getActTexId(), _shader, _tfo);

	quad->draw(_tfo);
}

//----------------------------------------------------

void SNTestVideoCaptureV4L::update(double time, double dt)
{
	// setCtrl take A LOT time, only change it if necessary
	if ( lastBrightness != brightness)
	{
		lastBrightness = brightness;
		vt->setCtrl("Brightness", brightness);
	}
	if ( lastGain != gain)
	{
		lastGain = gain;
		vt->setCtrl("Gain", gain);
	}
	if ( lastSaturation != saturation)
	{
		lastSaturation = saturation;
		vt->setCtrl("Saturation", saturation);
	}
	if ( lastHue != hue)
	{
		lastHue = hue;
		vt->setCtrl("Hue", hue);
	}

	vt->loadFrameToTexture();
}

//----------------------------------------------------

SNTestVideoCaptureV4L::~SNTestVideoCaptureV4L()
{
	delete vt;
	delete quad;
}

}
