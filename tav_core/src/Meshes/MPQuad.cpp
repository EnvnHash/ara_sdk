//
//  MPQuad.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  creates a MPQuad consisting of two triangles
//  x, y means the lower left corner

#include "pch.h"
#include "Meshes/MPQuad.h"

namespace tav
{
MPQuad::MPQuad()
{
	width = 1.f;
	height = 1.f;

	r = 1.0f;
	g = 1.0f;
	b = 1.0f;
	a = 1.0f;

	// first triangle, lower left, starting in the upper left corner,
	// counterclockwise
	position.push_back(glm::vec3(-0.5f, 0.5f, 0.f));
	texCoords.push_back(glm::vec2(0.f, 1.f));

	position.push_back(glm::vec3(-0.5f, -0.5f, 0.f));
	texCoords.push_back(glm::vec2(0.f, 0.f));

	position.push_back(glm::vec3(0.5f, -0.5f, 0.f));
	texCoords.push_back(glm::vec2(1.f, 0.f));

	// second triangle, right upper, starting in the lower right corner,
	// counterclockwise
	position.push_back(glm::vec3(0.5f, -0.5f, 0.f));
	texCoords.push_back(glm::vec2(0.f, 0.f));

	position.push_back(glm::vec3(0.5f, 0.5f, 0.f));
	texCoords.push_back(glm::vec2(1.f, 1.f));

	position.push_back(glm::vec3(-0.5f, 0.5f, 0.f));
	texCoords.push_back(glm::vec2(0.f, 1.f));

	// set normal facing outwards of the screen
	for (auto i = 0; i < 6; i++)
		normal.push_back(glm::vec3(0.f, 0.f, 1.f));

	init();
}

//---------------------------------------------------------------

MPQuad::MPQuad(float x, float y, float w, float h)
{
	width = w;
	height = h;

	r = 1.0f;
	g = 1.0f;
	b = 1.0f;
	a = 1.0f;

	glm::vec3 upperLeft = glm::vec3(x, y + h, 0.f);
	glm::vec3 lowerLeft = glm::vec3(x, y, 0.f);
	glm::vec3 lowerRight = glm::vec3(x + w, y, 0.f);
	glm::vec3 upperRight = glm::vec3(x + w, y + h, 0.f);

	// first triangle, lower left, starting in the upper left corner,
	// counterclockwise
	position.push_back(upperLeft);
	texCoords.push_back(glm::vec2(0.f, 1.f));

	position.push_back(lowerLeft);
	texCoords.push_back(glm::vec2(0.f, 0.f));

	position.push_back(lowerRight);
	texCoords.push_back(glm::vec2(1.f, 0.f));

	// second triangle, right upper, starting in the lower right corner,
	// counterclockwise
	position.push_back(lowerRight);
	texCoords.push_back(glm::vec2(1.f, 0.f));

	position.push_back(upperRight);
	texCoords.push_back(glm::vec2(1.f, 1.f));

	position.push_back(upperLeft);
	texCoords.push_back(glm::vec2(0.f, 1.f));

	// set normal facing outwards of the screen
	for (auto i = 0; i < 6; i++)
		normal.push_back(glm::vec3(0.f, 0.f, 1.f));

	init();
}

//---------------------------------------------------------------

MPQuad::MPQuad(float x, float y, float w, float h, glm::vec3 inNormal, float _r,
		float _g, float _b, float _a)
{
	width = w;
	height = h;

	r = _r;
	g = _g;
	b = _b;
	a = _a;

	glm::vec3 upperLeft = glm::vec3(x, y + h, 0.f);
	glm::vec3 lowerLeft = glm::vec3(x, y, 0.f);
	glm::vec3 lowerRight = glm::vec3(x + w, y, 0.f);
	glm::vec3 upperRight = glm::vec3(x + w, y + h, 0.f);

	// get the rotation matrix for the projection from inNormal to the standard normal
	glm::quat rot = RotationBetweenVectors(glm::vec3(0.f, 0.f, 1.f), inNormal);
	glm::mat4 rMatr;
	// special case if inNormal is opposite of standard normal
	if (inNormal != glm::vec3(0.f, 0.f, -1.f))
		rMatr = glm::mat4_cast(rot);
	else
		rMatr = glm::rotate(glm::mat4(1.f), static_cast<float>(M_PI),
				glm::vec3(0.f, 1.f, 0.f));

	// apply the matrix
	glm::vec4 tempV;
	tempV = (rMatr * glm::vec4(upperLeft, 1.f));
	upperLeft = glm::vec3(tempV.x, tempV.y, tempV.z);

	tempV = rMatr * glm::vec4(lowerLeft, 1.f);
	lowerLeft = glm::vec3(tempV.x, tempV.y, tempV.z);

	tempV = rMatr * glm::vec4(lowerRight, 1.f);
	lowerRight = glm::vec3(tempV.x, tempV.y, tempV.z);

	tempV = rMatr * glm::vec4(upperRight, 1.f);
	upperRight = glm::vec3(tempV.x, tempV.y, tempV.z);

	// first triangle, lower left, starting in the upper left corner,
	// counterclockwise
	position.push_back(upperLeft);
	texCoords.push_back(glm::vec2(0.f, 1.f));

	position.push_back(lowerLeft);
	texCoords.push_back(glm::vec2(0.f, 0.f));

	position.push_back(lowerRight);
	texCoords.push_back(glm::vec2(1.f, 0.f));

	// second triangle, right upper, starting in the lower right corner,
	// counterclockwise
	position.push_back(lowerRight);
	texCoords.push_back(glm::vec2(1.f, 0.f));

	position.push_back(upperRight);
	texCoords.push_back(glm::vec2(1.f, 1.f));

	position.push_back(upperLeft);
	texCoords.push_back(glm::vec2(0.f, 1.f));

	// set normal facing outwards of the screen
	for (auto i = 0; i < 6; i++)
		normal.push_back(inNormal);

	init();
}

//---------------------------------------------------------------

MPQuad::~MPQuad()
{
}

//---------------------------------------------------------------

void MPQuad::init()
{
	format = "position:3f,normal:3f,texCoord:2f,color:4f";
	mesh = new Mesh(format);

	// move the MPQuad to the correct position, in relation to the dimensions
	for (auto i = 0; i < 6; i++)
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

void MPQuad::remove()
{
	mesh->remove();
}
}
