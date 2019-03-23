//
//  MPCubeElem.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  creates a MPCubeElem consisting of two triangles_strips
//  has to be drawn with drawElements
//
//  origin is (0|0|0), size is (2|2|2)
//

#include "pch.h"
#include "Meshes/MPCubeElem.h"

namespace tav
{
MPCubeElem::MPCubeElem(float _width, float _height, float _depth, float _r,
		float _g, float _b, float _a) :
		width(_width), height(_height), depth(_depth)
{
	r = _r;
	g = _g;
	b = _b;
	a = _a;

	// position
	GLfloat cube_positions[] =
	{ -1.0f, 1.0f, -1.0f,  // left, upper, back
			1.0f, 1.0f, -1.0f,  // right, upper, back
			-1.0f, 1.0f, 1.0f,  // left, upper, front
			1.0f, 1.0f, 1.0f,  // right, upper, front
			-1.0f, -1.0f, -1.0f,   // left, lower, back
			1.0f, -1.0f, -1.0f,   // right, lower, back
			-1.0f, -1.0f, 1.0f,   // left, lower, front
			1.0f, -1.0f, 1.0f    // right, lower, front
			};

	// normal
	GLfloat cube_normals[] =
	{ -0.57735f, 0.57735f, -0.57735f,  // left, upper, back
			0.57735f, 0.57735f, -0.57735f,  // right, upper, back
			-0.57735f, 0.57735f, 0.57735f,  // left, upper, front
			0.57735f, 0.57735f, 0.57735f,  // right, upper, front
			-0.57735f, -0.57735f, -0.57735f,  // left, lower, back
			0.57735f, -0.57735f, -0.57735f,  // right, lower, back
			-0.57735f, -0.57735f, 0.57735f,  // left, lower, front
			0.57735f, -0.57735f, 0.57735f,  // right, lower, front
			};

	// Color for each vertex
	GLfloat tex_coords[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
			0.0f, 0.0f, 0.5f, 0.5f, };

	GLushort cube_indices[] =
	{ 1, 5, 0, 4, 2, 6, 3, 7,      // First strip
			0xFFFF,                     // <<-- This is the restart index
			0, 2, 1, 3, 5, 7, 4, 6      // Second strip
			};

	format = "position:3f,normal:3f,texCoord:2f";
	mesh = new Mesh(format);

	mesh->push_back_indices(cube_indices,
			sizeof(cube_indices) / sizeof(GLushort));
	mesh->push_back_positions(cube_positions,
			sizeof(cube_positions) / sizeof(GLfloat));
	mesh->push_back_normals(cube_normals,
			sizeof(cube_normals) / sizeof(GLfloat));
	mesh->push_back_texCoords(tex_coords, sizeof(tex_coords) / sizeof(GLfloat));
}

//---------------------------------------------------------------

MPCubeElem::~MPCubeElem()
{
}

//---------------------------------------------------------------

void MPCubeElem::remove()
{
	mesh->remove();
}
}
