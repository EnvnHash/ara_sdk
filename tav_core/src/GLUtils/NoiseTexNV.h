/*
 * NoiseTexNV.h
 *
 *  Created on: 16.09.2016
 *      Copyright by Sven Hahne
 */

#ifndef NOISETEXNV_H_
#define NOISETEXNV_H_

#pragma once

#include <stdlib.h>
#include "headers/gl_header.h"

namespace tav
{

class NoiseTexNV
{
public:
	NoiseTexNV(int w, int h, int d, GLint _internalFormat);
	~NoiseTexNV();
	GLuint getTex();

private:
	int width;
	int height;
	int depth;
	GLint internalFormat;
	GLuint tex;
};

}

#endif /* NOISETEXNV_H_ */
