//
//  GeoPrimitive.h
//  tav_gl4
//
//  Created by Sven Hahne on 10.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include "Meshes/Mesh.h"
#include "GLUtils/VAO.h"
#ifndef __EMSCRIPTEN__
#include "GLUtils/TFO.h"
#endif

namespace tav
{

class GeoPrimitive
{
public:
	GeoPrimitive();
	virtual ~GeoPrimitive();
	virtual void init() = 0;
	virtual void remove() = 0;
#ifndef __EMSCRIPTEN__
	virtual void draw(TFO* _tfo = nullptr);
	virtual void draw(GLenum _type, TFO* _tfo = nullptr, GLenum _recMode =
			GL_TRIANGLES);
	virtual void draw(GLenum _mode, unsigned int offset, unsigned int count,
			TFO* _tfo, GLenum _recMode);
	virtual void drawInstanced(int nrInstances, TFO* _tfo, float nrVert);
	virtual void drawInstanced(GLenum _type, int nrInstances, TFO* _tfo,
			float nrVert = 1.f);
	virtual void drawElements(GLenum _type, TFO* _tfo = nullptr,
			GLenum _recMode = GL_TRIANGLES);
	void drawElementsInst(GLenum _mode, int nrInstances, TFO* _tfo=NULL, GLenum _recMode=GL_TRIANGLES);
#else
	virtual void draw();
	virtual void draw(GLenum _type, GLenum _recMode=GL_TRIANGLES);
	virtual void draw(GLenum _mode, unsigned int offset, unsigned int count, GLenum _recMode);
	virtual void drawInstanced(int nrInstances, float nrVert=1.f);
	virtual void drawInstanced(GLenum _type, int nrInstances, float nrVert=1.f);
	virtual void drawElements(GLenum _type, GLenum _recMode=GL_TRIANGLES);
#endif
	virtual void drawElements();
	virtual void setColor(float _r, float _g, float _b, float _a);
	virtual void setAlpha(float _a);
	virtual void scale(float scaleX, float scaleY, float scaleZ);
	virtual void rotate(float angle, float rotX, float rotY, float rotZ);
	virtual void translate(float _x, float _y, float _z);
	virtual void invertNormals();
#ifndef __EMSCRIPTEN__
	virtual void* getMapBuffer(coordType _attrIndex);
	virtual void unMapBuffer();
#endif
	virtual int getNrVert();

	Mesh* mesh = nullptr;
	VAO* vao = nullptr;
	GLfloat r;
	GLfloat g;
	GLfloat b;
	GLfloat a;
	const char* format;
};
}
