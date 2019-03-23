//
//  MaterialProperties.h
//  tav_gl4
//
//  Created by Sven Hahne on 16.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <cstring>
#include <glm/glm.hpp>

#include "headers/gl_header.h"

namespace tav
{
class ShaderProperties
{
public:
	ShaderProperties();
	~ShaderProperties();

	struct ParType
	{
		bool isSet;
		int location;
		bool bVal;
		float fVal;
		GLfloat* f3Val;
		GLfloat* f4Val;
		std::string type;
		std::string name;
	};

	virtual void init(int _nrPar);
	virtual void sendToShader(GLuint _prog);
	virtual void sendStructToShader(GLuint _prog, std::string _name);
	virtual void sendStructArrayToShader(GLuint _prog, std::string _name,
			int _index);
	virtual void sendUniform(ParType* _type);

	int nrPar;
	ParType* parTypes;
};
}
