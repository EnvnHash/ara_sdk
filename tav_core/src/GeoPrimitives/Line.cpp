//
//  Line.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 21.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "GeoPrimitives/Line.h"

namespace tav
{
Line::Line() :
		GeoPrimitive(), nrSegments(100), instAttribs(nullptr), maxNrInstances(1)
{
	r = 1.0f;
	g = 1.0f;
	b = 1.0f;
	a = 1.0f;
	init();
}

//------------------------------------------------------

Line::Line(int _nrSegments) :
		GeoPrimitive(), nrSegments(_nrSegments), instAttribs(nullptr), maxNrInstances(
				1)
{
	r = 1.0f;
	g = 1.0f;
	b = 1.0f;
	a = 1.0f;
	init();
}

//------------------------------------------------------

Line::Line(int _nrSegments, float _r, float _g, float _b, float _a) :
		GeoPrimitive(), nrSegments(_nrSegments), instAttribs(nullptr), maxNrInstances(
				1)
{
	r = _r;
	g = _g;
	b = _b;
	a = _a;
	init();
}

//------------------------------------------------------

Line::Line(int _nrSegments, float _r, float _g, float _b, float _a,
		std::vector<coordType>* _instAttribs, int _nrInstance) :
		GeoPrimitive(), nrSegments(_nrSegments), instAttribs(_instAttribs), maxNrInstances(
				_nrInstance)
{
	r = _r;
	g = _g;
	b = _b;
	a = _a;
	init();
}

//------------------------------------------------------

Line::~Line()
{
}

//------------------------------------------------------

void Line::init()
{
	format = "position:3f,color:4f";
	mesh = new Mesh(format);

	for (int i = 0; i < nrSegments; i++)
	{
		float fInd = static_cast<float>(i) / static_cast<float>(nrSegments - 1);

		// move the quad to the correct position, in relation to the dimensions
		GLfloat v[3] = { fInd * 2.f - 1.0f, 0.f, 0.f };
		mesh->push_back_positions(v, 3);
	}

	GLenum usage = GL_DYNAMIC_DRAW;
	if (instAttribs)
		usage = GL_DYNAMIC_DRAW;

	vao = new VAO(format, usage, instAttribs, maxNrInstances);
	vao->setStaticColor(r, g, b, a);
	vao->uploadMesh(mesh);
}

//------------------------------------------------------

void Line::remove()
{
	vao->remove();
	mesh->remove();
}
}
