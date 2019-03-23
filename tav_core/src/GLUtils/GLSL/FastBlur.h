//
//  FastBlur.h
//  Tav_App
//
//  Created by Sven Hahne on 23/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__FastBlur__
#define __Tav_App__FastBlur__

#pragma once

#include <stdio.h>
#include "GLUtils/PingPongFbo.h"
#include "Shaders/Shaders.h"
#include "Shaders/ShaderCollector.h"
#include "Communication/OSC/OSCData.h"

namespace tav
{
class FastBlur
{
public:
	FastBlur(OSCData* _osc, ShaderCollector* _shCol, int _blurW, int _blurH,
			GLenum _type = GL_RGBA8);
	FastBlur(ShaderCollector* _shCol, int _blurW, int _blurH, GLenum _type);
	~FastBlur();
	void init();
	void proc(GLint texIn);
	void downloadData();
	void initShader();
	GLint getResult();
	unsigned char* getData();
	unsigned short* getDataR16();
private:
	OSCData* osc;
	ShaderCollector* shCol;
	Quad* fboQuad;
	GLuint depthTexNr;
	GLuint colorTexNr;
	GLuint showTexNr = 0;
	GLenum type;

	Shaders* linearV;
	Shaders* linearH;
	Shaders* shdr;

	PingPongFbo* pp;

	int frameNr = -1;
	int width;
	int height;
	int blurW;
	int blurH;

	float fWidth;
	float fHeight;
	float fAlpha;

	GLfloat* blurOffsH;
	GLfloat* blurOffsV;
	float offsScale;
	float blFdbk;

	bool updateProc = false;
	bool useOsc = false;

	unsigned short* data16;
	unsigned char* data;
};
}
#endif /* defined(__Tav_App__FastBlur__) */
