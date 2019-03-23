//
// SNTestQuad.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNMask.h"

namespace tav
{
SNMask::SNMask(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	getChanCols(&chanCols);

	quad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f),
			chanCols[0].r, chanCols[0].g, chanCols[0].b, chanCols[0].a);

	stdCol = shCol->getStdCol();
}

//---------------------------------------------------------------

SNMask::~SNMask()
{
	delete quad;
}

//---------------------------------------------------------------

void SNMask::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);
	}

	//sendStdShaderInit(_shader);
	// sendPvmMatrix(_shader, cp);

	//quad->setAlpha(alpha);

	glDisable (GL_BLEND);
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);

	stdCol->begin();
	stdCol->setUniformMatrix4fv("m_pvm", &_modelMat[0][0]);
	quad->draw(_tfo);
}

//---------------------------------------------------------------

void SNMask::update(double time, double dt)
{
}

}
