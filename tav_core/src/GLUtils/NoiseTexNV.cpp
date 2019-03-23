/*
 * NoiseTexNV.cpp
 *
 *  Created on: 16.09.2016
 *      Copyright by Sven Hahne
 *
 *  noise texture with values from -1 to 1
 */

#define STRINGIFY(A) #A

#include "pch.h"
#include "NoiseTexNV.h"

namespace tav
{
NoiseTexNV::NoiseTexNV(int w, int h, int d, GLint _internalFormat) :
		width(w), height(h), depth(d), internalFormat(_internalFormat)
{
	uint8_t *data = new uint8_t[w * h * d * 4];
	uint8_t *ptr = data;
	for (int z=0; z<d; z++)
	{
		for (int y=0; y<h; y++)
		{
			for (int x=0; x<w; x++)
			{
				*ptr++ = rand() & 0xff;
				*ptr++ = rand() & 0xff;
				*ptr++ = rand() & 0xff;
				*ptr++ = rand() & 0xff;
			}
		}
	}

	glGenTextures(1, &tex);
	getGlError();
	glBindTexture(GL_TEXTURE_3D, tex);
	getGlError();

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	getGlError();
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	getGlError();
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	getGlError();
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	getGlError();
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	getGlError();

	//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, w, h, d, 0, GL_RGBA, GL_BYTE,
			data);
	getGlError();

	delete[] data;
}

//--------------------------------------------------------------------------------

GLuint NoiseTexNV::getTex()
{
	return tex;
}

//--------------------------------------------------------------------------------

NoiseTexNV::~NoiseTexNV()
{
}
}
