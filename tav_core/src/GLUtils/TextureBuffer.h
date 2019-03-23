//
//  TextureBuffer.h
//  tav_gl4
//
//  Created by Sven Hahne on 27.09.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#ifndef __tav_gl4__TextureBuffer__
#define __tav_gl4__TextureBuffer__

#include <cstring>
#include <stdio.h>

#include "headers/gl_header.h"
#include "GLUtils/VAO.h"

namespace tav
{
class TextureBuffer
{
public:
	TextureBuffer(unsigned int _size, unsigned int _nrCoords,
			GLfloat* _initData = NULL);
	~TextureBuffer();
	GLfloat* getMapBuffer();
	void unMapBuffer();
	void upload(unsigned int _size, GLfloat* _data, unsigned int _offs = 0);
	VAO* getVao();
	GLuint getTex();
	GLuint getBuf();
	GLenum getStoreMode();
	void bindTex(GLuint _unit = 0);
	void makeVao();
	void clear();

private:
	unsigned int size;
	unsigned int nrCoords;
	GLuint buf;
	GLuint tex;
	GLenum storeMode;
	GLenum intFormat;
	GLenum extFormat;
	GLfloat* nullData;
	VAO* vao;
};
}

#endif /* defined(__tav_gl4__TextureBuffer__) */
