//
// GodRays.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>

#include "GeoPrimitives/Circle.h"
#include "GLUtils/FBO.h"
#include "GeoPrimitives/Quad.h"

namespace tav
{

class GodRays
{
public:
	GodRays(ShaderCollector* _shCol, unsigned int _width, unsigned int _height,
			unsigned int _nrSamples = 80, GLenum _type = GL_RGBA8);
	~GodRays();

	void initShdr();
	void bind();
	void unbind();
	void draw();
	void drawLight(glm::vec4* lightCol);
	void setAlpha(float _val);
	void setExposure(float _val);
	void setDensity(float _val);
	void setDecay(float _val);
	void setWeight(float _val);
	void setLightPosScr(float _x, float _y);

private:
	ShaderCollector* shCol;
	FBO* godRaysFbo;

	Quad* quad;
	Quad* quadNoFlip;
	Circle* lightCircle;

	Shaders* godRayShdr;
	Shaders* stdColShdr;
	Shaders* stdTex;
	Shaders* maskShader;
	Shaders* applyMaskShader;

	glm::vec3 lightPos;
	glm::vec2 lightPosScreen;

	unsigned int width;
	unsigned int height;
	unsigned int nrSamples;

	float alpha;
	float exposure;
	float decay;
	float density;
	float weight;
};
}
