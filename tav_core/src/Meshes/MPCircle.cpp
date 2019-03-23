//
//  MPCircle.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  creates a MPCircle consisting of two triangles
//  x, y means the lower left corner

#include "pch.h"
#include "MPCircle.h"

namespace tav
{
MPCircle::MPCircle()
{
	outerRadius = 0.5f;
	innerRadius = 0.1f;
	smoothNorm = 0.5f;

	nrSegQuads = 20;
	angle = TWO_PI;
	closeCircle = true;

	r = 1.0f;
	g = 1.0f;
	b = 1.0f;
	a = 1.0f;
	init();
}

MPCircle::MPCircle(int _nrSegs, float _outerRad, float _innerRad, float _angle,
		float _r, float _g, float _b, float _a)
{
	outerRadius = _outerRad;
	innerRadius = _innerRad;
	nrSegQuads = _nrSegs;
	angle = _angle;
	r = _r;
	g = _g;
	b = _b;
	a = _a;

	smoothNorm = 0.5f;
	closeCircle = angle >= TWO_PI ? true : false;
	init();
}

//---------------------------------------------------------------

MPCircle::~MPCircle()
{
}

//---------------------------------------------------------------

void MPCircle::init()
{
	//        int adjNrSeg = nrSegQuads;
	//        if (!closeCircle) adjNrSeg -= 1;

	for (auto i = 0; i < nrSegQuads + 1; i++)
	{
		float fInd = static_cast<float>(i) / static_cast<float>(nrSegQuads);

		outerRing.push_back(
				glm::vec3(std::cos(fInd * angle) * outerRadius,
						std::sin(fInd * angle) * outerRadius, 0.f));

		innerRing.push_back(
				glm::vec3(std::cos(fInd * angle) * innerRadius,
						std::sin(fInd * angle) * innerRadius, 0.f));
	}

	// gehe die skelett punkte durch, konstruiere jeweils ein Quad
	// bestehend aus zwei Triangles pro Segment

	for (int i = 0; i < nrSegQuads; i++)
	{
		float fInd = static_cast<float>(i) / static_cast<float>(nrSegQuads);
		float fIndPlusOne = static_cast<float>(i + 1)
				/ static_cast<float>(nrSegQuads);

		position.push_back(outerRing[i]);
		texCoords.push_back(glm::vec2(fInd, 1.f));
		normal.push_back(
				glm::vec3(0.f, 0.f, 1.f) * (1.f - smoothNorm)
						+ (glm::normalize(outerRing[i]) * smoothNorm));

		position.push_back(innerRing[(i + 1)]);
		texCoords.push_back(glm::vec2(fIndPlusOne, 0.f));
		normal.push_back(glm::vec3(0.f, 0.f, 1.f));

		position.push_back(innerRing[i]);
		texCoords.push_back(glm::vec2(fInd, 0.f));
		normal.push_back(glm::vec3(0.f, 0.f, 1.f));

		//-----

		position.push_back(innerRing[(i + 1)]);
		texCoords.push_back(glm::vec2(fIndPlusOne, 0.f));
		normal.push_back(glm::vec3(0.f, 0.f, 1.f));

		position.push_back(outerRing[i]);
		texCoords.push_back(glm::vec2(fInd, 1.f));
		normal.push_back(
				glm::vec3(0.f, 0.f, 1.f) * (1.f - smoothNorm)
						+ (glm::normalize(outerRing[i]) * smoothNorm));

		position.push_back(outerRing[(i + 1)]);
		texCoords.push_back(glm::vec2(fIndPlusOne, 1.f));
		normal.push_back(
				glm::vec3(0.f, 0.f, 1.f) * (1.f - smoothNorm)
						+ (glm::normalize(outerRing[(i + 1)]) * smoothNorm));
	}

	format = "position:3f,normal:3f,texCoord:2f,color:4f";
	mesh = new Mesh(format);

	for (auto i = 0; i < nrSegQuads * 6; i++)
	{
		GLfloat v[3] =
		{ position[i].x, position[i].y, position[i].z };
		mesh->push_back_positions(v, 3);

		GLfloat n[3] =
		{ normal[i].x, normal[i].y, normal[i].z };
		mesh->push_back_normals(n, 3);

		GLfloat t[2] =
		{ texCoords[i].x, texCoords[i].y };
		mesh->push_back_texCoords(t, 2);
	}
}

//---------------------------------------------------------------

void MPCircle::remove()
{
	mesh->remove();
}

//---------------------------------------------------------------

int MPCircle::getNrSegments()
{
	return nrSegQuads;
}
}
