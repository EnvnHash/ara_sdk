//
//  FastBlurMem.h
//  Tav_App
//
//  Created by Sven Hahne on 23/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__FastBlurMem__
#define __Tav_App__FastBlurMem__

#pragma once

#include <stdio.h>
#include "GLUtils/FBO.h"
#include "GLUtils/PingPongFbo.h"
#include "Shaders/Shaders.h"
#include "Shaders/ShaderCollector.h"
#include "Communication/OSC/OSCData.h"

namespace tav
{
class FastBlurMem
{
public:

	enum blurKernelSize { KERNEL_3, KERNEL_5, NUM_BLUR_KERNELS };

	FastBlurMem(float _alpha, ShaderCollector* _shCol, int _blurW, int _blurH,
			GLenum _type = GL_RGBA8, bool _rot180 = false, blurKernelSize _kSize=KERNEL_3);
	~FastBlurMem();
	void proc(GLint texIn, unsigned int itNr=0);
	void initShader();
	GLint getResult();
	GLint getLastResult();
	void setAlpha(float _alpha);
	void setBright(float _bright);
	void setOffsScale(float _offsScale);
	void setPVM(GLfloat* _pvm_ptr);

private:
	OSCData* osc;
	ShaderCollector* shCol;
	Quad* fboQuad;

	GLfloat* pvm_ptr = 0;
	GLuint depthTexNr;
	GLuint colorTexNr;
	GLuint showTexNr = 0;

	Shaders* linearV;
	Shaders* linearH;
	Shaders* shdr;

	FBO* firstPassFbo;
	PingPongFbo* pp;

	int frameNr = -1;
	int width;
	int height;
	int blurW;
	int blurH;

	unsigned int actKernelSize;
	unsigned int nrRandOffs;

	float fWidth;
	float fHeight;

	float alpha;
	float bright = 1.f;
	float offsScale = 1.f;

	GLfloat* blurOffs;
	GLfloat* blurOffsScale;

	bool updateProc = false;
	bool rot180 = false;

	blurKernelSize kSize;
};
}
#endif /* defined(__Tav_App__FastBlurMem__) */
