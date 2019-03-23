//
//  Room.h
//  tav_gl4
//
//  Created by Sven Hahne on 16.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <vector>
#include <cmath>
#include <glm/glm.hpp>

#include "GeoPrimitives/GeoPrimitive.h"

namespace tav
{
class Sphere: public GeoPrimitive
{
public:
	Sphere(float _radius, int _nrSlices, bool _cclockw = true,
			bool _triangulate = true, bool _genTexCoord = true);
	~Sphere();
	void init();
#ifndef __EMSCRIPTEN__
	void draw(TFO* _tfo = nullptr);
#else
	void draw();
#endif
	void remove();
	GLfloat* getPositions();
	GLfloat* getNormals();
	GLfloat* getTexCoords();
	GLfloat* getColors();
	GLuint* getIndices();
	unsigned int getNrVertices();
private:
	bool genTexCoord;
	bool cclockw;
	bool triangulate;
	float radius;
	std::vector<coordType>* instAttribs = nullptr;
	int maxNrInstances = 1;
	unsigned int numberSlices;
	const unsigned int MAX_VERTICES = 1048576;
	const unsigned int MAX_INDICES = MAX_VERTICES * 4;

	unsigned int numberVertices;

	GLfloat* vertices;
	GLfloat* normals;
	GLfloat* texCoords;
	GLfloat* colors;
	GLuint* indices;

	GLfloat* tVertices;
	GLfloat* tNormals;
	GLfloat* tTexCoords;
	GLfloat* tColors;
};
}
