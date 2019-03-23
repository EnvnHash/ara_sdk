//
//  ShaderCollector.h
//  Tav_App
//
//  Created by Sven Hahne on 4/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__ShaderCollector__
#define __Tav_App__ShaderCollector__

#pragma once

#include <stdio.h>
#include <cstring>
#include <map>
#include "Shaders/Shaders.h"

namespace tav
{
class ShaderCollector
{
public:
	ShaderCollector();
	~ShaderCollector();
	void addShader(std::string _name, Shaders* _shader);
	Shaders* addCheckShader(std::string _name, const char* vert, const char* frag);
	Shaders* addCheckShader(std::string _name, const char* vert, const char* geom, const char* frag);
	Shaders* addCheckShader(std::string _name, const char* vert,
			const char* cont, const char* eval, const char* geom,
			const char* frag);
	Shaders* addCheckShaderText(std::string _name, const char* comp);
	Shaders* addCheckShaderText(std::string _name, const char* vert, const char* frag);
	Shaders* addCheckShaderText(std::string _name, const char* vert, const char* geom, const char* frag);
	Shaders* addCheckShaderText(std::string _name, const char* vert,
			const char* cont, const char* eval, const char* geom,
			const char* frag);
	Shaders* addCheckShaderTextNoLink(std::string _name, const char* comp);
	Shaders* addCheckShaderTextNoLink(std::string _name, const char* vert, const char* frag);
	Shaders* addCheckShaderTextNoLink(std::string _name, const char* vert,
			const char* geom, const char* frag);
	Shaders* getStdClear(bool layered = false, int nrLayers = 1);
	Shaders* getStdCol();
	Shaders* getStdParCol();
	Shaders* getStdColAlpha();
	Shaders* getStdColBorder();
	Shaders* getStdDirLight();
	Shaders* getStdRec();
	Shaders* getStdTex();
	Shaders* getStdTexMulti();
	Shaders* getStdTexAlpha(bool multiSampTex = false);
	Shaders* getEdgeDetect();
	Shaders* getStdHeightMapSobel();
	Shaders* getPerlin();

	std::string getShaderHeader();

	Shaders* get(std::string _name);
	bool hasShader(std::string _name);

private:
	std::map<std::string, Shaders*> shaderCollection;
	std::string shdr_Header;
};
}

#endif /* defined(__Tav_App__ShaderCollector__) */
