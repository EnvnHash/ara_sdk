//
//  ShaderAssembler.h
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <vector>
#include <boost/regex.hpp>
#include <cstring>
#include <iterator>
#include <sstream>
#include <boost/algorithm/string.hpp>

#include "headers/tav_types.h"
#include "GLUtils/glm_utils.h"

namespace tav
{
typedef struct
{
	std::string name = "";
	std::string type = "";
} uniformInput;

typedef struct
{
	std::string arg = "";
	std::string name = "";
	std::string type = "";
} layoutInput;

typedef struct
{
	std::string arg = "";
	std::string name = "";
	std::string type = "";
} layoutOutput;

typedef struct
{
	std::string dir = "";
	std::string type = "";
	std::string name = "";
} varType;

typedef struct
{
	std::string version;
	shaderType type;
	std::string code;
	std::string pCode;
	std::vector<uniformInput*> uniformInputs;
	std::vector<varType*> outputs;
	std::vector<layoutInput*> layoutInputs;
	std::vector<layoutOutput*> layoutOutputs;
} shaderDescr;

class ShaderAssembler
{
public:
	ShaderAssembler();
	~ShaderAssembler();

	void setWorkShader(shaderType _type);

	void setVersion(std::string& _in);
	void addLayoutIn(std::string _arg, std::string _type, std::string _name);
	void addLayoutOut(std::string _arg, std::string _type, std::string _name);
	void addUniform(std::string _type, std::string _name);
	void addCode(std::string _code);

	// vertex shader functions
	void setVertStdInput();
	void setEnableInstancing();
	void setInstModMatr();
	void setFragStdInput();
	void setGeomStdInput();
	void setUniformSeparateMVP();

	const char* getVertShader();
	const char* getFragShader();
	const char* getGeomShader();

	void asmblMultiCamGeo(int nrCams);
	void removeStdProj();
	void assemble(shaderType _shdrType, bool assembleRaw);
	void setInputs(std::string& srcString, std::string inPrfx,
			std::stringstream& dst);
	void getOutputsFromVert();
	void addEdgeBlendCodeHead2Head();
	void addEdgeBlendCode2();
	void addEdgeBlendCode3();
	void addEdgeBlendCodeN(int nrCams);
	void print();
	// std::string parseCode(std::regex_constants::error_type etype);
private:
	std::stringstream shdr;
	shaderDescr* descr;
	shaderDescr* wD;
	std::string* resShader;
	shaderType lastShdrType;
};
}
