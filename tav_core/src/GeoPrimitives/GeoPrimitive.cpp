//
//  GeoPrimitive.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 10.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "GeoPrimitives/GeoPrimitive.h"

namespace tav
{
GeoPrimitive::GeoPrimitive()
{
}

GeoPrimitive::~GeoPrimitive()
{
	delete vao;
	delete mesh;
}

//------------------------------------------------------------------------

void GeoPrimitive::setColor(float _r, float _g, float _b, float _a)
{
	r = _r;
	g = _g;
	b = _b;
	a = _a;
	this->vao->setStaticColor(r, g, b, a);
}

void GeoPrimitive::setAlpha(float _a)
{
	this->vao->setStaticColor(r, g, b, _a);
}

//------------------------------------------------------------------------

#ifndef __EMSCRIPTEN__
void GeoPrimitive::draw(TFO* _tfo)
{
	this->vao->draw(GL_TRIANGLES, _tfo, GL_TRIANGLES);
}

void GeoPrimitive::draw(GLenum _type, TFO* _tfo, GLenum _recMode)
{
	this->vao->draw(_type, _tfo, _recMode);
}

void GeoPrimitive::draw(GLenum _mode, unsigned int offset, unsigned int count,
		TFO* _tfo, GLenum _recMode)
{
	this->vao->draw(_mode, offset, count, _tfo, _recMode);
}

void GeoPrimitive::drawInstanced(int nrInstances, TFO* _tfo, float nrVert)
{
	this->vao->drawInstanced(GL_TRIANGLES, nrInstances, _tfo, nrVert);
}

void GeoPrimitive::drawInstanced(GLenum _type, int nrInstances, TFO* _tfo,
		float nrVert)
{
	this->vao->drawInstanced(_type, nrInstances, _tfo, nrVert);
}

void GeoPrimitive::drawElements(GLenum _type, TFO* _tfo, GLenum _recMode)
{
	this->vao->drawElements(_type, _tfo, _recMode);
}

void GeoPrimitive::drawElementsInst(GLenum _mode, int nrInstances, TFO* _tfo,
		GLenum _recMode)
{
	this->vao->drawElementsInst(_mode, nrInstances, _tfo,_recMode);
}

#else
void GeoPrimitive::draw()
{
	this->vao->draw(GL_TRIANGLES);
}

void GeoPrimitive::draw(GLenum _type, GLenum _recMode)
{
	this->vao->draw(_type);
}

void GeoPrimitive::draw(GLenum _mode, unsigned int offset, unsigned int count, GLenum _recMode)
{
	this->vao->draw(_mode, offset, count, _recMode);
}

void GeoPrimitive::drawInstanced(int nrInstances, float nrVert)
{
	this->vao->drawInstanced(GL_TRIANGLES, nrInstances, nrVert);
}

void GeoPrimitive::drawInstanced(GLenum _type, int nrInstances, float nrVert)
{
	this->vao->drawInstanced(_type, nrInstances, nrVert);
}

void GeoPrimitive::drawElements(GLenum _type, GLenum _recMode)
{
	this->vao->drawElements(_type, _recMode);
}
#endif

void GeoPrimitive::drawElements()
{
	this->vao->drawElements(GL_TRIANGLES);
}

//------------------------------------------------------------------------

void GeoPrimitive::scale(float scaleX, float scaleY, float scaleZ)
{
	if (this->mesh != nullptr && this->vao != nullptr)
	{
		this->mesh->scale(scaleX, scaleY, scaleZ);
		this->vao->uploadMesh(this->mesh);
	}
}

void GeoPrimitive::rotate(float angle, float rotX, float rotY, float rotZ)
{
	if (this->mesh != nullptr && this->vao != nullptr)
	{
		this->mesh->rotate(angle, rotX, rotY, rotZ);
		this->vao->uploadMesh(this->mesh);
	}
}

void GeoPrimitive::translate(float _x, float _y, float _z)
{
	if (this->mesh != nullptr && this->vao != nullptr)
	{
		this->mesh->translate(_x, _y, _z);
		this->vao->uploadMesh(this->mesh);
	}
}

void GeoPrimitive::invertNormals()
{
	this->mesh->invertNormals();
	this->vao->uploadMesh(this->mesh);
}

#ifndef __EMSCRIPTEN__
//------------------------------------------------------------------------

void* GeoPrimitive::getMapBuffer(coordType _attrIndex)
{
	return this->vao->getMapBuffer(_attrIndex);
}

//------------------------------------------------------------------------

void GeoPrimitive::unMapBuffer()
{
	this->vao->unMapBuffer();
}
#endif
//------------------------------------------------------------------------

int GeoPrimitive::getNrVert()
{
	return this->vao->getNrVertices();
}
}
