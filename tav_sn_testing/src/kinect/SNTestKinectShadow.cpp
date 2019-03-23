//
//  SNTestKinectShadow.cpp
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "SNTestKinectShadow.h"

namespace tav
{
SNTestKinectShadow::SNTestKinectShadow(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs)
{
	kin = static_cast<KinectInput*>(scd->kin);
	kin->setUpdateShadow(true);

	quad = scd->stdQuad;
}

//----------------------------------------------------

void SNTestKinectShadow::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_CULL_FACE); // wenn enabled wird die "rueckseite" des quad nicht gezeichnet...
	glDisable(GL_DEPTH_TEST);

	sendStdShaderInit(_shader);
	useTextureUnitInd(0, kin->getShdwTexId(), _shader, _tfo);

	quad->draw(_tfo);
}

//----------------------------------------------------

void SNTestKinectShadow::update(double time, double dt)
{
	kin->uploadShadowImg();
}

//----------------------------------------------------

SNTestKinectShadow::~SNTestKinectShadow()
{
	delete quad;
}
}
