//
//  TextureBuffer.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 27.09.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  work in progress...

#include "pch.h"
#include "GLUtils/TextureBuffer.h"

namespace tav
{
TextureBuffer::TextureBuffer(unsigned int _size, unsigned int _nrCoords,
		GLfloat* _initData) :
		size(_size), nrCoords(_nrCoords), storeMode(GL_DYNAMIC_DRAW)
{
	glGenBuffers(1, &buf);
	glBindBuffer(GL_TEXTURE_BUFFER, buf);
	glBufferData(GL_TEXTURE_BUFFER, size * _nrCoords * sizeof(GLfloat),
			_initData, storeMode);

	// Now create the buffer texture and associate it
	// with the buffer object.
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_BUFFER, tex);
	switch (_nrCoords)
	{
	case 1:
		intFormat = GL_R32F;
		extFormat = GL_RED;
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, buf);
		break;
	case 2:
		intFormat = GL_RG32F;
		extFormat = GL_RG;
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, buf);
		break;
	case 3:
		intFormat = GL_RGB32F;
		extFormat = GL_RGB;
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, buf);
		break;
	case 4:
		intFormat = GL_RGBA32F;
		extFormat = GL_RGBA;
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buf);
		break;
	default:
		break;
	}

	glBindTexture(GL_TEXTURE_BUFFER, 0);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);

	nullData = new GLfloat[size * _nrCoords];
	memset(nullData, 0.f, size * _nrCoords);
}

//---------------------------------------------------------------------

TextureBuffer::~TextureBuffer()
{
}

//---------------------------------------------------------------------

GLfloat* TextureBuffer::getMapBuffer()
{
	glBindBuffer(GL_TEXTURE_BUFFER, buf);
	return (GLfloat*) glMapBuffer(GL_TEXTURE_BUFFER, GL_WRITE_ONLY);
}

//---------------------------------------------------------------------

void TextureBuffer::unMapBuffer()
{
	glUnmapBuffer(GL_TEXTURE_BUFFER);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

//---------------------------------------------------------------------

void TextureBuffer::upload(unsigned int _size, GLfloat* _data,
		unsigned int _offs)
{
	glBindBuffer(GL_TEXTURE_BUFFER, buf);
	glBufferSubData(GL_TEXTURE_BUFFER, _offs,
			_size * nrCoords * sizeof(GLfloat), _data);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

//---------------------------------------------------------------------

VAO* TextureBuffer::getVao()
{
	return vao;
}

//---------------------------------------------------------------------

GLuint TextureBuffer::getTex()
{
	return tex;
}

//---------------------------------------------------------------------

GLuint TextureBuffer::getBuf()
{
	return buf;
}

//---------------------------------------------------------------------

GLenum TextureBuffer::getStoreMode()
{
	return storeMode;
}

//---------------------------------------------------------------------

void TextureBuffer::bindTex(GLuint _unit)
{
	glActiveTexture(GL_TEXTURE0 + _unit);
	glBindTexture(GL_TEXTURE_BUFFER, tex);
}

//---------------------------------------------------------------------
// use textureBuffer always as "position"
void TextureBuffer::makeVao()
{
	vao = new VAO("position:4f", storeMode, false);
	vao->addExtBuffer(POSITION, buf);
}

//---------------------------------------------------------------------

void TextureBuffer::clear()
{
	// glClearBufferData gibt´s erst ab 4.3 ...
	glBindBuffer(GL_TEXTURE_BUFFER, buf);
	glBufferData(GL_TEXTURE_BUFFER, size * nrCoords * sizeof(GLfloat), nullData,
			GL_DYNAMIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
}
}
