//
// SNClearScreen.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNClearScreen.h"

namespace tav
{
SNClearScreen::SNClearScreen(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "NoLight")
{
	osc = static_cast<OSCData*>(scd->osc);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	clearShader = shCol->getStdClear();
}

//---------------------------------------------------------------

SNClearScreen::~SNClearScreen()
{
}

//---------------------------------------------------------------

void SNClearScreen::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glClear (GL_DEPTH_BUFFER_BIT);   // necessary, sucks performance
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);   // necessary, sucks performance

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask (GL_FALSE);

	clearCol.r = osc->backColor;
	clearCol.g = osc->backColor;
	clearCol.b = osc->backColor;
	clearCol.a = 1.f - osc->feedback;

	clearShader->begin();
	clearShader->setIdentMatrix4fv("m_pvm");
	clearShader->setUniform4fv("clearCol", &clearCol[0], 1);
	_stdQuad->draw();

	glDepthMask (GL_TRUE);
	;
}

//---------------------------------------------------------------

void SNClearScreen::update(double time, double dt)
{
}
}
