//
// SNTestVideoCapture.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestVideoCapture.h"

namespace tav
{

SNTestVideoCapture::SNTestVideoCapture(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    				SceneNode(_scd, _sceneArgs,"NoLight")
{
	vt = new VideoTextureCapture((char*) "/dev/video0", 30);

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);
	quad->rotate(M_PI, 1.f, 0.f, 0.f);
}

//----------------------------------------------------

void SNTestVideoCapture::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_CULL_FACE);
	sendStdShaderInit(_shader);

	useTextureUnitInd(0, vt->texIDs[0], _shader, _tfo);

	quad->draw(_tfo);
}

//----------------------------------------------------

void SNTestVideoCapture::update(double time, double dt)
{
	vt->loadFrameToTexture();
}

//----------------------------------------------------

SNTestVideoCapture::~SNTestVideoCapture()
{
	delete vt;
	delete quad;
}

}
