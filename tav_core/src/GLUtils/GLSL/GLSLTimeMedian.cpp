/*
 * GLSLTimeMedian.cpp
 *
 *  Created on: 13.12.2016
 *      Copyright by Sven Hahne
 */

//
//  GLSLTimeMedian.cpp
//  tav_core
//
//  Created by Sven Hahne on 13/7/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "GLUtils/GLSL/GLSLTimeMedian.h"

#define STRINGIFY(A) #A

namespace tav
{
GLSLTimeMedian::GLSLTimeMedian(ShaderCollector* _shCol, int _width, int _height,
		GLenum _intFormat) :
		shCol(_shCol), width(_width), height(_height), srcId(0), median(3.f)
{
	initShader(shCol);
	texShader = _shCol->getStdTex();

	texture = new PingPongFbo(shCol, width, height, _intFormat, GL_TEXTURE_2D,
			false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);

	quad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 0.f);
}

//---------------------------------------------------------

void GLSLTimeMedian::initShader(ShaderCollector* _shCol)
{
	//------ Position Shader -----------------------------------------

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 2 ) in vec2 texCoord; out vec2 tex_coord; void main() { tex_coord = texCoord; gl_Position = position; });

	vert = "// Time Median Shader pos tex vertex shader\n" + shdr_Header + vert;

	//------ Frag Shader -----------------------------------------

	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string frag =
			STRINGIFY(
					uniform sampler2D act; uniform sampler2D last; uniform float median; in vec2 tex_coord; layout (location = 0) out vec4 color;

					void main() { color = (texture(act, tex_coord) + texture(last, tex_coord) * median) / (median +1.0); });

	frag = "// Time Median Shader pos tex shader\n" + shdr_Header + frag;

	medShader = _shCol->addCheckShaderText("StandardTimeMedian", vert.c_str(),
			frag.c_str());
}

//---------------------------------------------------------

void GLSLTimeMedian::update(GLint tex)
{
	glEnable(GL_BLEND);

	texture->dst->bind();
	texture->dst->clear();

	if (isInited)
	{
		medShader->begin();
		medShader->setIdentMatrix4fv("m_pvm");
		medShader->setUniform1i("act", 0);
		medShader->setUniform1i("last", 1);
		medShader->setUniform1f("median", median);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture->getSrcTexId());

	}
	else
	{
		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);

		isInited = true;
	}

	quad->draw();

	texture->dst->unbind();
	texture->swap();
}

//---------------------------------------------------------

void GLSLTimeMedian::setMedian(float _median)
{
	median = _median;
}

//---------------------------------------------------------

void GLSLTimeMedian::setPVM(GLfloat* _pvm_ptr)
{
	pvm_ptr = _pvm_ptr;

}

//---------------------------------------------------------

GLuint GLSLTimeMedian::getResTexId()
{
	return texture->getSrcTexId();
}

//---------------------------------------------------------

GLuint GLSLTimeMedian::getLastResId()
{
	return texture->getDstTexId();
}

//---------------------------------------------------------

GLSLTimeMedian::~GLSLTimeMedian()
{
	medShader->remove();
	delete medShader;
	delete quad;
	delete texture;
}
}
