//
//  ShaderProperties.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 16.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "ShaderProperties.h"

namespace tav
{
ShaderProperties::ShaderProperties()
{
}

ShaderProperties::~ShaderProperties()
{
}

void ShaderProperties::init(int _nrPar)
{
	nrPar = _nrPar;

	parTypes = new ParType[nrPar];
	for (int i = 0; i < nrPar; i++)
	{
		parTypes[i].isSet = false;
		parTypes[i].location = -1;
		parTypes[i].bVal = false;
		parTypes[i].fVal = 0.0f;
		parTypes[i].f3Val = new GLfloat[3];
		for (int j = 0; j < 3; j++)
			parTypes[i].f3Val[j] = 0.0f;
		parTypes[i].f4Val = new GLfloat[4];
		for (int j = 0; j < 4; j++)
			parTypes[i].f4Val[j] = 0.0f;
	}
}

void ShaderProperties::sendToShader(GLuint _prog)
{
	for (int i = 0; i < nrPar; i++)
	{
		if (parTypes[i].isSet)
		{
			if (parTypes[i].location == -1)
				parTypes[i].location = glGetUniformLocation(_prog,
						parTypes[i].name.c_str());

			sendUniform(&parTypes[i]);
		}
	}
}

void ShaderProperties::sendStructToShader(GLuint _prog, std::string _name)
{
	for (int i = 0; i < nrPar; i++)
	{
		if (parTypes[i].isSet)
		{
			if (parTypes[i].location == -1)
				parTypes[i].location = glGetUniformLocation(_prog,
						(_name + "." + parTypes[i].name).c_str());

			sendUniform(&parTypes[i]);
		}
	}
}

void ShaderProperties::sendStructArrayToShader(GLuint _prog, std::string _name,
		int _index)
{
	for (int i = 0; i < nrPar; i++)
	{
		if (parTypes[i].isSet)
		{
			if (parTypes[i].location == -1)
				parTypes[i].location = glGetUniformLocation(_prog,
						(_name + "[" + std::to_string(_index) + "]."
								+ parTypes[i].name).c_str());
			sendUniform(&parTypes[i]);
		}
	}
}

void ShaderProperties::sendUniform(ParType* _type)
{
	if (std::strcmp(_type->type.c_str(), "b") == 0)
	{
		glUniform1i(_type->location, _type->bVal);

	}
	else if (std::strcmp(_type->type.c_str(), "f") == 0)
	{
		glUniform1f(_type->location, _type->fVal);

	}
	else if (std::strcmp(_type->type.c_str(), "3fv") == 0)
	{
		glUniform3fv(_type->location, 1, _type->f3Val);

	}
	else if (std::strcmp(_type->type.c_str(), "4fv") == 0)
	{
//            std::cout << "parTypes Ptr: " << _type << std::endl;
//            std::cout << "ShaderProperties::sendUniform: " << _type->name.c_str() << std::endl;
//            printf("%f %f %f %f \n", _type->f4Val[0], _type->f4Val[1], _type->f4Val[2], _type->f4Val[3] );
//            std::cout << std::endl;

		glUniform4fv(_type->location, 1, _type->f4Val);
	}
}
}
