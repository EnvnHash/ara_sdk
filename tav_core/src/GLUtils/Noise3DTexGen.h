//
//  3DNoiseTexgen.h
//  Tav_App
//
//  Created by Sven Hahne on 18/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App___DNoiseTexgen__
#define __Tav_App___DNoiseTexgen__

#include <cstdlib>
#include <cmath>
#include <stdio.h>

#include "headers/gl_header.h"
#include "Shaders/ShaderCollector.h"
#include "GLUtils/FBO.h"
#include "Shaders/Shaders.h"
#include "GeoPrimitives/Quad.h"

namespace tav
{
class Noise3DTexGen
{
public:
	Noise3DTexGen(ShaderCollector* _shCol, bool color, int nrOctaves,
			int _width, int _height, int _depth, float _scaleX, float _scaleY,
			float _scaleZ);
	~Noise3DTexGen();
	void initShdr();
	void initBlendShdr();
	GLuint getTex();

	FBO* fbo;
	FBO* xBlendFboH;
	FBO* xBlendFboV;

private:

	Shaders* noiseShdr;
	Shaders* xBlendShaderH;
	Shaders* xBlendShaderV;
	Shaders* stdTexShdr;
	ShaderCollector* shCol;
	Quad* quad;
	Quad* fboQuad;

	GLfloat* zPos;
	float scaleX;
	float scaleY;
	float scaleZ;

	int width;
	int height;
	int depth;
	int nrLoops;
	short nrParallelTex;
};
}

#endif /* defined(__Tav_App___DNoiseTexgen__) */
