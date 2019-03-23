//
//  UniformBlock.h
//  tav_core
//
//  Created by Sven Hahne on 22/7/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//


#pragma once

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <stdio.h>
#include <utility>

#include "headers/gl_header.h"
#include "GLUtils/glm_utils.h"

namespace tav
{
typedef struct
{
	std::string name = "";
	GLenum inType;
	float* fVal;
	bool* bVal;
	unsigned int* uVal;
	int* iVal;
	void* vVal;
	GLuint indices;
	GLint size;
	GLint offset;
	GLint type;
} ubBlockVar;

class UniformBlock
{
public:
	UniformBlock(GLuint _program, std::string _blockName);
	~UniformBlock();
	void bind();
	void unbind();

//        void addVarName(std::string _name, int* _inVal);
//        void addVarName(std::string _name, float* _inVal);
//        void addVarName(std::string _name, bool* _inVal);
//        void addVarName(std::string _name, unsigned int* _inVal);
	void addVarName(std::string _name, void* _inVal, GLenum _type);
	void changeVarName(std::string _name, void* _inVal, GLenum _type);

	void update();
	size_t TypeSize(GLenum type);
private:
	std::vector<ubBlockVar*> valPairs;

	GLuint uboIndex;
	GLint uboSize;
	GLuint ubo = 0;
	GLubyte* buffer;

	GLuint* indices;
	GLint* size;
	GLint* offset;
	GLint* type;

	GLuint program;
	int numUniforms;
	const char** names;
};
}
