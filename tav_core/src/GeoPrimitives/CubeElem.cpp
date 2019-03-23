//
//  CubeElem.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  needs glEnable(GL_PRIMITIVE_RESTART); glPrimitiveRestartIndex(0xFFFF);
//  minimum memory usage, only 8 vertices needed
//

#include "pch.h"
#include "CubeElem.h"

namespace tav
{
CubeElem::CubeElem(float _width, float _height, float _depth) :
		GeoPrimitive(), width(_width), height(_height), depth(_depth), instAttribs(
				nullptr), maxNrInstances(0)
{
	r = 1.0f;
	g = 1.0f;
	b = 1.0f;
	a = 1.0f;

	init();
}

//----------------------------------------------------------------------------------------

CubeElem::CubeElem(float _width, float _height, float _depth,
		std::vector<coordType>* _instAttribs, int _maxNrInstances, float _r,
		float _g, float _b, float _a) :
		GeoPrimitive(), width(_width), height(_height), depth(_depth), instAttribs(
				_instAttribs), maxNrInstances(_maxNrInstances)
{
	r = _r;
	g = _g;
	b = _b;
	a = _a;

	init();
}

//----------------------------------------------------------------------------------------

CubeElem::~CubeElem()
{
	delete mCubeElem;
}

//----------------------------------------------------------------------------------------

void CubeElem::init()
{
	mCubeElem = new MPCubeElem(width, height, depth, r, g, b, a);
	mesh = mCubeElem->getMesh();
	format = mCubeElem->getFormat();

	GLenum usage = GL_STATIC_DRAW;
	if (instAttribs)
		usage = GL_DYNAMIC_DRAW;

	vao = new VAO(format, usage, instAttribs, maxNrInstances);
	vao->uploadMesh(mesh);
}

//----------------------------------------------------------------------------------------

void CubeElem::draw(TFO* _tfo)
{
//        vao->draw(GL_TRIANGLE_STRIP);
	vao->drawElements(GL_TRIANGLE_STRIP, _tfo, GL_TRIANGLES);
}

//----------------------------------------------------------------------------------------

void CubeElem::remove()
{
	vao->remove();
	mesh->remove();
	delete mCubeElem;
}

}
