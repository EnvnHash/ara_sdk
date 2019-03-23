/*
 * GLSLTimeMedian.h
 *
 *  Created on: 13.12.2016
 *      Copyright by Sven Hahne
 */

#ifndef GLSL_GLSLTIMEMEDIAN_H_
#define GLSL_GLSLTIMEMEDIAN_H_

#pragma once

#include <stdio.h>

#include "GeoPrimitives/Quad.h"
#include "GLUtils/PingPongFbo.h"
#include "Shaders/ShaderCollector.h"
#include "GLUtils/TextureManager.h"

#define STRINGIFY(A) #A

namespace tav
{
class GLSLTimeMedian
{
public:
	GLSLTimeMedian(ShaderCollector* _shCol, int _width, int _height,
			GLenum _intFormat);
	~GLSLTimeMedian();
	void initShader(ShaderCollector* _shCol);
	void update(GLint tex);
	void setMedian(float _median);
	void setPVM(GLfloat* _pvm_ptr);
	GLuint getResTexId();
	GLuint getLastResId();

private:
	ShaderCollector* shCol;
	Shaders* medShader;
	Shaders* texShader;
	Quad* quad;
	PingPongFbo* texture;

	bool isInited = false;

	int width;
	int height;

	GLuint srcId;
	GLuint lastSrcId;
	GLfloat* pvm_ptr = 0;

	float median;
};
}

#endif /* GLSL_GLSLTIMEMEDIAN_H_ */
