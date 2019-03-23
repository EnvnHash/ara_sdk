//
//  Disk.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  draw as triangle fan
//

#include "pch.h"
#include "Disk.h"

namespace tav
{
Disk::Disk(float _width, float _height, int _nrSubDiv,
		std::vector<coordType>* _instAttribs, int _maxNrInstances, float _r,
		float _g, float _b, float _a) :
		GeoPrimitive(), width(_width), height(_height), nrSubDiv(_nrSubDiv), instAttribs(
				nullptr), maxNrInstances(1)
{
	r = _r;
	g = _g;
	b = _b;
	a = _a;

	init();
}

//----------------------------------------------------------------------------------------

Disk::~Disk()
{
}

//----------------------------------------------------------------------------------------

void Disk::init()
{
	double alpha;

	format = "position:3f,normal:3f,texCoord:2f,color:4f";
	GLenum usage = GL_STATIC_DRAW;
	if (instAttribs)
		usage = GL_DYNAMIC_DRAW;

	vao = new VAO(format, usage, instAttribs, maxNrInstances);

	// positions and texcoords
	GLfloat* positions = new GLfloat[(nrSubDiv + 2) * 3];
	GLfloat* texCoords = new GLfloat[(nrSubDiv + 2) * 2];

	// center
	for (int i = 0; i < 3; i++)
		positions[i] = 0.f;
	for (int i = 0; i < 2; i++)
		texCoords[i] = 0.5f;

	for (int i = 0; i < (nrSubDiv + 1); i++)
	{
		alpha = static_cast<double>(i) / static_cast<double>(nrSubDiv) * M_PI
				* 2.0;
		positions[(i + 1) * 3] = std::cos(alpha) * width * 0.5;
		positions[(i + 1) * 3 + 1] = std::sin(alpha) * width * 0.5;
		positions[(i + 1) * 3 + 2] = 0.f;

		texCoords[(i + 1) * 2] = std::cos(alpha) * 0.5 + 0.5;
		texCoords[(i + 1) * 2 + 1] = std::sin(alpha) * 0.5 + 0.5;
	}

	// normals
	GLfloat* normals = new GLfloat[(nrSubDiv + 2) * 3];
	for (int i = 0; i < (nrSubDiv + 2); i++)
	{
		normals[i * 3] = 0.f;
		normals[i * 3 + 1] = 0.f;
		normals[i * 3 + 2] = 1.f;
	}

	// colors
	GLfloat* colors = new GLfloat[(nrSubDiv + 2) * 4];
	for (int i = 0; i < (nrSubDiv + 2) * 4; i++)
		colors[i] = 1.f;

	vao->upload(POSITION, positions, (nrSubDiv + 2));
	vao->upload(NORMAL, normals, (nrSubDiv + 2));
	vao->upload(TEXCOORD, texCoords, (nrSubDiv + 2));
	vao->upload(COLOR, colors, (nrSubDiv + 2));
}

//----------------------------------------------------------------------------------------

void Disk::remove()
{
	vao->remove();
}

}
