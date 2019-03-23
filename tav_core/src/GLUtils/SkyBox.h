//
//  SkyBox.h
//  Tav_App
//
//  Created by Sven Hahne on 18/3/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#ifndef __Tav_App__SkyBox__
#define __Tav_App__SkyBox__

#include <stdio.h>
#include <cstdlib>
#include <string>

#include "headers/tav_types.h"
#include "GeoPrimitives/Sphere.h"
#include "Shaders/Shaders.h"
#include "GLUtils/TextureManager.h"
#include "GLUtils/TFO.h"

namespace tav
{
class SkyBox
{
public:
	SkyBox(std::string textureFile, unsigned _nrCams = 1);
	SkyBox(const char* textureFile, unsigned _nrCams = 1);
	~SkyBox();
	void init(const char* textureFile, unsigned _nrCams);
	void draw(double time, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void setMatr(glm::mat4* _inMatr);
	void remove();
private:
	Sphere* sphere;
	TextureManager* cubeTex;
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

#endif /* defined(__Tav_App__SkyBox__) */
