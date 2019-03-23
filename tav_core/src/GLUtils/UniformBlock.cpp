//
//  UniformBlock.cpp
//  tav_core
//
//  Created by Sven Hahne on 22/7/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//
//  should be used after the shader is linked
//  uniform block has to be used in shader, otherwise the compiler optimizes it, means kill it

#include "pch.h"
#include "GLUtils/UniformBlock.h"

namespace tav
{
UniformBlock::UniformBlock(GLuint _program, std::string _blockName) :
		program(_program), uboIndex(0)
{
	glUseProgram(_program);

	// Find the uniform buffer index for "Uniforms", and
	// determine the block’s sizes
	uboIndex = glGetUniformBlockIndex(_program, _blockName.c_str());

	if (uboIndex == GL_INVALID_INDEX)
	{
		printf("tav::UniformBlock Error: %d, couldn´t get the index of the requested Uniform Block \n", uboIndex);
	}
	else
	{
		glGetActiveUniformBlockiv(_program, uboIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &uboSize);
		buffer = (GLubyte*) malloc(uboSize);

		if (buffer == NULL)
			fprintf(stderr, "Unable to allocate buffer\n");
	}
}
/*
 //----------------------------------------------------------------------
 
 void UniformBlock::addVarName(std::string _name, int* _inVal)
 {
 valPairs.push_back( new ubBlockVar() );
 valPairs.back()->name = _name;
 valPairs.back()->iVal = _inVal;
 valPairs.back()->inType = GL_INT;
 }
 
 //----------------------------------------------------------------------
 
 void UniformBlock::addVarName(std::string _name, float* _inVal)
 {
 valPairs.push_back( new ubBlockVar() );
 valPairs.back()->name = _name;
 valPairs.back()->fVal = _inVal;
 valPairs.back()->inType = GL_FLOAT;
 }
 
 //----------------------------------------------------------------------
 
 void UniformBlock::addVarName(std::string _name, bool* _inVal)
 {
 valPairs.push_back( new ubBlockVar() );
 valPairs.back()->name = _name;
 valPairs.back()->bVal = _inVal;
 valPairs.back()->inType = GL_BOOL;
 }
 
 //----------------------------------------------------------------------
 
 void UniformBlock::addVarName(std::string _name, unsigned int* _inVal)
 {
 valPairs.push_back( new ubBlockVar() );
 valPairs.back()->name = _name;
 valPairs.back()->uVal = _inVal;
 valPairs.back()->inType = GL_UNSIGNED_INT;
 }
 */

//----------------------------------------------------------------------
void UniformBlock::addVarName(std::string _name, void* _inVal, GLenum _type)
{
	valPairs.push_back(new ubBlockVar());
	valPairs.back()->name = _name;
	valPairs.back()->inType = _type;

	switch (_type)
	{
	case GL_INT:
		valPairs.back()->iVal = static_cast<int*>(_inVal);
		break;
	case GL_FLOAT:
		valPairs.back()->fVal = static_cast<float*>(_inVal);
		break;
	case GL_UNSIGNED_INT:
		valPairs.back()->uVal = static_cast<unsigned int*>(_inVal);
		break;
	case GL_BOOL:
		valPairs.back()->bVal = static_cast<bool*>(_inVal);
		break;
	default:
		valPairs.back()->vVal = _inVal;
		break;
	}
}

//----------------------------------------------------------------------

void UniformBlock::changeVarName(std::string _name, void* _inVal, GLenum _type)
{
	bool found = false;
	std::vector<ubBlockVar*>::iterator item;
	for (std::vector<ubBlockVar*>::iterator it = valPairs.begin();
			it != valPairs.end(); ++it)
	{
		if (std::strcmp((*it)->name.c_str(), _name.c_str()) == 0)
		{
			found = true;
			item = it;
		}
	}

	if (found)
	{
		(*item)->name = _name;
		(*item)->inType = _type;

		switch (_type)
		{
		case GL_INT:
			(*item)->iVal = static_cast<int*>(_inVal);
			break;
		case GL_FLOAT:
			(*item)->fVal = static_cast<float*>(_inVal);
			break;
		case GL_UNSIGNED_INT:
			(*item)->uVal = static_cast<unsigned int*>(_inVal);
			break;
		case GL_BOOL:
			(*item)->bVal = static_cast<bool*>(_inVal);
			break;
		default:
			(*item)->vVal = _inVal;
			break;
		}
	}
	else
	{
		printf("UniformBlock::changeVarName Error, name not found \n");
	}
}

//----------------------------------------------------------------------

void UniformBlock::bind()
{
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBindBufferBase(GL_UNIFORM_BUFFER, uboIndex, ubo);
}

//----------------------------------------------------------------------

void UniformBlock::unbind()
{
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

//----------------------------------------------------------------------

void UniformBlock::update()
{
	if (buffer != NULL)
	{
		// Query the necessary attributes to determine
		// where in the buffer we should write the values
		numUniforms = static_cast<int>(valPairs.size());

		names = new const char*[numUniforms];
		for (auto i = 0; i < numUniforms; i++)
			names[i] = valPairs[i]->name.c_str();

		indices = new GLuint[numUniforms];
		size = new GLint[numUniforms];
		offset = new GLint[numUniforms];
		type = new GLint[numUniforms];

		glGetUniformIndices(program, numUniforms, names, indices);
		glGetActiveUniformsiv(program, numUniforms, indices, GL_UNIFORM_OFFSET,
				offset);
		glGetActiveUniformsiv(program, numUniforms, indices, GL_UNIFORM_SIZE,
				size);
		glGetActiveUniformsiv(program, numUniforms, indices, GL_UNIFORM_TYPE,
				type);

		// Copy the uniform values into the buffer
		for (auto i = 0; i < numUniforms; i++)
		{
			switch (valPairs[i]->inType)
			{
			case GL_INT:
				memcpy(buffer + offset[i], valPairs[i]->iVal,
						size[i] * TypeSize(type[i]));
				break;
			case GL_FLOAT:
				memcpy(buffer + offset[i], valPairs[i]->fVal,
						size[i] * TypeSize(type[i]));
				break;
			case GL_UNSIGNED_INT:
				memcpy(buffer + offset[i], valPairs[i]->uVal,
						size[i] * TypeSize(type[i]));
				break;
			case GL_BOOL:
				memcpy(buffer + offset[i], valPairs[i]->bVal,
						size[i] * TypeSize(type[i]));
				break;
			default:
				memcpy(buffer + offset[i], valPairs[i]->vVal,
						size[i] * TypeSize(type[i]));
				break;
			}
		}

		// Create the uniform buffer object, initialize
		// its storage, and associated it with the shader
		// program
		if (ubo == 0)
			glGenBuffers(1, &ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		glBufferData(GL_UNIFORM_BUFFER, uboSize, buffer, GL_STATIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, uboIndex, ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
}

//----------------------------------------------------------------------

size_t UniformBlock::TypeSize(GLenum type)
{
	size_t size;

#define CASE(Enum, Count, Type) \
case Enum: size = Count * sizeof(Type); break

	switch (type)
	{
	CASE(GL_FLOAT, 1, GLfloat)
;		CASE(GL_FLOAT_VEC2, 2, GLfloat);
		CASE(GL_FLOAT_VEC3, 3, GLfloat);
		CASE(GL_FLOAT_VEC4, 4, GLfloat);
		CASE(GL_INT, 1, GLint);
		CASE(GL_INT_VEC2, 2, GLint);
		CASE(GL_INT_VEC3, 3, GLint);
		CASE(GL_INT_VEC4, 4, GLint);
		CASE(GL_UNSIGNED_INT, 1, GLuint);
		CASE(GL_UNSIGNED_INT_VEC2, 2, GLuint);
		CASE(GL_UNSIGNED_INT_VEC3, 3, GLuint);
		CASE(GL_UNSIGNED_INT_VEC4, 4, GLuint);
		CASE(GL_BOOL, 1, GLboolean);
		CASE(GL_BOOL_VEC2, 2, GLboolean);
		CASE(GL_BOOL_VEC3, 3, GLboolean);
		CASE(GL_BOOL_VEC4, 4, GLboolean);
		CASE(GL_FLOAT_MAT2, 4, GLfloat);
		CASE(GL_FLOAT_MAT2x3, 6, GLfloat);
		CASE(GL_FLOAT_MAT2x4, 8, GLfloat);
		CASE(GL_FLOAT_MAT3, 9, GLfloat);
		CASE(GL_FLOAT_MAT3x2, 6, GLfloat);
		CASE(GL_FLOAT_MAT3x4, 12, GLfloat);
		CASE(GL_FLOAT_MAT4, 16, GLfloat);
		CASE(GL_FLOAT_MAT4x2, 8, GLfloat);
		CASE(GL_FLOAT_MAT4x3, 12, GLfloat);
#undef CASE
		default:
		fprintf(stderr, "Unknown type: 0x%x\n", type); exit(EXIT_FAILURE);
		break;
	}
	return size;
}

//----------------------------------------------------------------------

UniformBlock::~UniformBlock()
{
}
}
