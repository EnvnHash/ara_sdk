//
//  Circle.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  creates a Circle consisting of two triangles
//  x, y means the lower left corner

#include "pch.h"
#include "GeoPrimitives/Circle.h"

namespace tav
{
Circle::Circle() :
		GeoPrimitive()
{
	circ = new MPCircle();
	mesh = circ->getMesh();
	format = circ->getFormat();

	r = 1.f;
	g = 1.f;
	b = 1.f;
	a = 1.f;

	instAttribs = nullptr;
	maxNrInstances = 1;

	init();
}

//---------------------------------------------------------------

Circle::Circle(int _nrSegs, float _outerRad, float _innerRad, float _angle,
		float _r, float _g, float _b, float _a,
		std::vector<coordType>* _instAttribs, int _nrInstances) :
		GeoPrimitive()
{
	circ = new MPCircle(_nrSegs, _outerRad, _innerRad, _angle, _r, _g, _b, _a);
	mesh = circ->getMesh();
	format = circ->getFormat();

	r = _r;
	g = _g;
	b = _b;
	a = _a;

	instAttribs = _instAttribs;
	maxNrInstances = _nrInstances;

	init();
}

//---------------------------------------------------------------

Circle::~Circle()
{
	delete circ;
}

//---------------------------------------------------------------

void Circle::init()
{
	GLenum usage = GL_DYNAMIC_DRAW;
	if (instAttribs)
		usage = GL_DYNAMIC_DRAW;

	vao = new VAO(format, usage, instAttribs, maxNrInstances);
	vao->setStaticColor(r, g, b, a);
	vao->uploadMesh(mesh);
}

//---------------------------------------------------------------

void Circle::remove()
{
	vao->remove();
	circ->remove();
}
}
