//
// SNEnelMuevate.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNEnelMuevate.h"

namespace tav
{
SNEnelMuevate::SNEnelMuevate(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs)
{
	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
	tex0 = texs[ static_cast<GLuint>(_sceneArgs->at("tex0")) ];
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	quad = static_cast<Quad*>(_scd->stdQuad);

	stdTexAlpha = shCol->getStdTexAlpha();

	// get tex0
	img0Propo = texs[0]->getHeightF() / texs[0]->getWidthF();

	addPar("alpha", &alpha); // WIEDER AENDERN!!!
}

//----------------------------------------------------

SNEnelMuevate::~SNEnelMuevate()
{
}

//----------------------------------------------------

void SNEnelMuevate::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//std::cout << glm::to_string(_modelMat) << std::endl;

	glm::mat4 pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat
			* glm::scale(glm::vec3(cp->actFboSize.y / cp->actFboSize.x, img0Propo, 1.f));

	stdTexAlpha->begin();
	stdTexAlpha->setUniform1i("tex", 0);
	stdTexAlpha->setUniform1f("alpha", 1.f);
	stdTexAlpha->setUniformMatrix4fv("m_pvm", &pvm[0][0]);

	// draw tex0
	tex0->bind(0);
	scd->stdQuad->draw(_tfo);

	_shader->begin();
}

//----------------------------------------------------

void SNEnelMuevate::update(double time, double dt)
{
}

}
