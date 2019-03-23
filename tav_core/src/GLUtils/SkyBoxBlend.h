//
//  SkyBoxBlend.h
//  Tav_App
//
//  Created by Sven Hahne on 18/3/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#ifndef __Tav_App__SkyBoxBlend__
#define __Tav_App__SkyBoxBlend__

#include <stdio.h>
#include <cstdlib>
#include <string>

#include "headers/tav_types.h"
#include "../GeoPrimitives/Sphere.h"
#include "Shaders/Shaders.h"
#include "GLUtils/TextureManager.h"
#include "GLUtils/TFO.h"

namespace tav
{
class SkyBoxBlend
{
public:
	SkyBoxBlend(std::string textureFile, unsigned _nrCams = 1);
	SkyBoxBlend(const char* textureFile, unsigned _nrCams = 1);
	~SkyBoxBlend();
	void init(const char* textureFile, unsigned _nrCams);
	// Shaders* getPerlin();
	void draw(double time, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void setMatr(glm::mat4* _inMatr);
	void remove();
private:
	Sphere* sphere;
	TextureManager* cubeTex;
	Shaders* perlinShader;
	Shaders* sbShader;
	Shaders* testShader;
	std::string vShader;
	std::string gShader;
	std::string fShader;
	std::string vShaderSrc;
	GLuint texUnit;
	glm::mat4 pvm;
	float angle;
	glm::mat4* modMatr = NULL;
};
}

#endif /* defined(__Tav_App__SkyBoxBlend__) */
