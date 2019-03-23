//
//  GLSLOpticalFlow.h
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__GLSLOpticalFlow__
#define __Tav_App__GLSLOpticalFlow__

#pragma once

#include <stdio.h>

#include "GeoPrimitives/Quad.h"
#include "GLUtils/PingPongFbo.h"
#include "Shaders/ShaderCollector.h"
#include "GLUtils/TextureManager.h"

#define STRINGIFY(A) #A

namespace tav
{
class GLSLOpticalFlow
{
public:
	GLSLOpticalFlow(ShaderCollector* _shCol, int _width, int _height);
	~GLSLOpticalFlow();
	void initShader(ShaderCollector* _shCol);
	void update(GLint tex1, GLint tex2, float fdbk = 0.f);
	void setMedian(float _median);
	void setBright(float _val);
	void setPVM(GLfloat* _pvm_ptr);
	GLuint getResTexId();

private:
	ShaderCollector* shCol;
	Shaders* flowShader;
	Shaders* texShader;
	Quad* quad;
	PingPongFbo* texture;

	int width;
	int height;
	unsigned int isInited = 0;

	GLuint srcId;
	GLuint lastSrcId;
	GLfloat* pvm_ptr = 0;
	float lambda;
	float median;
	float bright;
};
}

#endif /* defined(__Tav_App__GLSLOpticalFlow__) */
