//
// SNTestFBO.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestFBO.h"

namespace tav
{
SNTestFBO::SNTestFBO(sceneData* _scd, std::map<std::string, float>* _sceneArgs) : SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	fbo = new FBO(shCol, 1024, 768, GL_RGBA8, GL_TEXTURE_2D,
			false, 1, 1, 1, GL_REPEAT, false);

	stdTex = shCol->getStdTex();

	testTex = new TextureManager();
	testTex->loadTexture2D((*scd->dataPath)+"textures/test.jpg");
}

//----------------------------------------------------

void SNTestFBO::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);
	}

	stdTex->begin();
	stdTex->setUniform1i("tex", 0);
	stdTex->setIdentMatrix4fv("m_pvm");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fbo->getColorImg());

	quad->draw();
}

//----------------------------------------------------

void SNTestFBO::update(double time, double dt)
{
	fbo->bind();
	fbo->clear();

	stdTex->begin();
	stdTex->setUniform1i("tex", 0);
	stdTex->setIdentMatrix4fv("m_pvm");
	testTex->bind(0);

	quad->draw();

	fbo->unbind();
}

//----------------------------------------------------

SNTestFBO::~SNTestFBO()
{
	delete fbo;
	delete quad;
	delete testTex;
}

}
