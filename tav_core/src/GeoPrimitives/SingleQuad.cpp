//
//  SingleQuad.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 07.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "SingleQuad.h"

namespace tav
{
SingleQuad::SingleQuad() :
		GeoPrimitive()
{
	x = -1.f;
	y = -1.f;
	width = 2.f;
	height = 2.f;
	r = 0.f;
	g = 0.f;
	b = 0.f;
	a = 1.f;
	init();
}

SingleQuad::SingleQuad(float _x, float _y, float _width, float _height) :
		GeoPrimitive()
{
	x = _x;
	y = _y;
	width = _width;
	height = _height;
	r = 1.f;
	g = 1.f;
	b = 1.f;
	a = 1.f;
	init();
}

SingleQuad::SingleQuad(float _x, float _y, float _width, float _height,
		float _r, float _g, float _b, float _a) :
		GeoPrimitive()
{
	x = _x;
	y = _y;
	width = _width;
	height = _height;
	r = _r;
	g = _g;
	b = _b;
	a = _a;
	init();
}

SingleQuad::SingleQuad(float _x, float _y, float _width, float _height,
		float _r, float _g, float _b, float _a, bool _flipH, bool _flipV) :
		GeoPrimitive()
{
	x = _x;
	y = _y;
	width = _width;
	height = _height;
	r = _r;
	g = _g;
	b = _b;
	a = _a;
	flipH = _flipH;
	flipV = _flipV;
	init();
}

SingleQuad::~SingleQuad()
{
}

void SingleQuad::init()
{
	format = "position:3f,texCoord:2f";
	GLfloat positions[] =
	{ x, y + height, 0.0f, x, y, 0.0f, x + width, y + height, 0.0f, x + width,
			y, 0.0f };

	GLfloat texCoords[] =
	{ 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f, 0.f };

	if (flipH)
	{
		texCoords[1] = 0.f;
		texCoords[3] = 1.f;
		texCoords[5] = 0.f;
		texCoords[7] = 1.f;
	}

	if (flipV)
	{
		texCoords[0] = 1.f;
		texCoords[2] = 1.f;
		texCoords[4] = 0.f;
		texCoords[6] = 0.f;
	}

	mesh = new Mesh(format);
	mesh->push_back_positions(positions, 12);
	mesh->push_back_texCoords(texCoords, 8);

	vao = new VAO(format, GL_STATIC_DRAW);
	vao->setStaticColor(r, g, b, a);
	vao->setStaticNormal(0.0f, 0.0f, 1.0f);
	vao->uploadMesh(mesh);
}

void SingleQuad::draw()
{
	vao->draw(GL_TRIANGLE_STRIP, nullptr, GL_TRIANGLE_STRIP);
}

void SingleQuad::draw(GLenum _type)
{
	vao->draw(_type, nullptr, _type);
}

void SingleQuad::remove()
{
	mesh->remove();
	vao->remove();
}
}
