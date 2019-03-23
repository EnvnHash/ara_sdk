//
//  QuadArrayAdj.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  creates an Array of Quads to be used with a geometry shader and input type triangles_adjacency
//  x, y means the lower left corner, nrSegs is per side
//  standard size is 2.f * 2.f from -1|-1|0 to 1|1|0

#include "pch.h"
#include "QuadArrayAdj.h"

namespace tav
{
QuadArrayAdj::QuadArrayAdj() :
		GeoPrimitive(), x(-1.f), y(-1.f), width(1.f), height(1.f), nrSegsX(4), nrSegsY(
				4), totalWidth(2.f), totalHeight(2.f), instAttribs(nullptr), maxNrInstances(
				1), usage(GL_STATIC_DRAW)
{
	qaNormal = glm::vec3(0.f, 0.f, 1.f);
	r = 1.0f;
	g = 1.0f;
	b = 1.0f;
	a = 1.0f;

	init();
}

//---------------------------------------------------------------

QuadArrayAdj::QuadArrayAdj(int _nrSegsX, int _nrSegsY, float _x, float _y,
		float _w, float _h, float _r, float _g, float _b, float _a,
		std::vector<coordType>* _instAttribs, int _nrInstances, GLenum _usage) :
		GeoPrimitive(), x(_x), y(_y), width(_w), height(_h), nrSegsX(_nrSegsX), nrSegsY(
				_nrSegsY), totalWidth(2.f), totalHeight(2.f), instAttribs(
				_instAttribs), maxNrInstances(_nrInstances), usage(_usage)
{
	qaNormal = glm::vec3(0.f, 0.f, 1.f);
	r = _r;
	g = _g;
	b = _b;
	a = _a;

	init();
}

//---------------------------------------------------------------

QuadArrayAdj::QuadArrayAdj(int _nrSegsX, int _nrSegsY, float _x, float _y,
		float _w, float _h, glm::vec3 inNormal,
		std::vector<coordType>* _instAttribs, int _nrInstances, GLenum _usage) :
		GeoPrimitive(), x(_x), y(_y), width(_w), height(_h), nrSegsX(_nrSegsX), nrSegsY(
				_nrSegsY), totalWidth(2.f), totalHeight(2.f), instAttribs(
				_instAttribs), maxNrInstances(_nrInstances), usage(_usage)
{
	qaNormal = inNormal;
	r = 0.0f;
	g = 0.0f;
	b = 0.0f;
	a = 1.0f;

	init();
}

//---------------------------------------------------------------

QuadArrayAdj::~QuadArrayAdj()
{
	delete vao;
}

//---------------------------------------------------------------

void QuadArrayAdj::init()
{
	GLfloat norm[3] =
	{ 0.f, 0.f, 1.f };

	float quadWidth = totalWidth / static_cast<float>(nrSegsX);
	float quadHeight = totalHeight / static_cast<float>(nrSegsY);

	float texWidth = 1.f / static_cast<float>(nrSegsX);
	float texHeight = 1.f / static_cast<float>(nrSegsY);

	format = "position:3f,normal:3f,texCoord:2f,color:4f";
	mesh = new Mesh(format);

	// make a grid with two additional quadSize horizontally and vertically
	for (auto yInd = 0; yInd < (nrSegsY + 3); yInd++)
	{
		for (auto xInd = 0; xInd < (nrSegsX + 3); xInd++)
		{
			GLfloat v[3] =
			{ quadWidth * (xInd - 1) + x, quadHeight * (yInd - 1) + y, 0.f };
			//printf("%d %f %f %f \n", yInd * (nrSegsX +3) + xInd, v[0], v[1], v[2]);

			mesh->push_back_positions(v, 3);
			mesh->push_back_normals(norm, 3);

			GLfloat t[2] =
			{ texWidth * (xInd - 1), texHeight * (yInd - 1) };
			//printf("%f %f \n", t[0], t[1]);
			mesh->push_back_texCoords(t, 2);
		}
	}

	// write indices
	for (auto yInd = 0; yInd < nrSegsY; yInd++)
	{
		for (auto xInd = 0; xInd < (nrSegsX * 2); xInd++)
		{
			if (xInd % 2 == 0)
			{
				GLushort ind[6] =
				{ static_cast<GLushort>((nrSegsY + 3) * (yInd + 1) + xInd / 2
						+ 1), static_cast<GLushort>((nrSegsY + 3) * (yInd + 2)
						+ xInd / 2), static_cast<GLushort>((nrSegsY + 3)
						* (yInd + 2) + xInd / 2 + 1),
						static_cast<GLushort>((nrSegsY + 3) * (yInd + 2)
								+ xInd / 2 + 2), static_cast<GLushort>((nrSegsY
								+ 3) * (yInd + 1) + xInd / 2 + 2),
						static_cast<GLushort>((nrSegsY + 3) * yInd + xInd / 2
								+ 2) };
				//printf("even y:%d: x:%d : %d %d %d %d %d %d\n", yInd, xInd, ind[0], ind[1], ind[2], ind[3], ind[4], ind[5]);
				mesh->push_back_indices(ind, 6);
			}
			else
			{
				GLushort ind[6] =
				{ static_cast<GLushort>((nrSegsY + 3) * (yInd + 2) + xInd / 2
						+ 1), static_cast<GLushort>((nrSegsY + 3) * (yInd + 3)
						+ xInd / 2 + 1), static_cast<GLushort>((nrSegsY + 3)
						* (yInd + 2) + xInd / 2 + 2),
						static_cast<GLushort>((nrSegsY + 3) * (yInd + 1)
								+ xInd / 2 + 3), static_cast<GLushort>((nrSegsY
								+ 3) * (yInd + 1) + xInd / 2 + 2),
						static_cast<GLushort>((nrSegsY + 3) * (yInd + 1)
								+ xInd / 2 + 1) };
				//printf("odd y:%d: x:%d : %d %d %d %d %d %d\n", yInd, xInd, ind[0], ind[1], ind[2], ind[3], ind[4], ind[5]);
				mesh->push_back_indices(ind, 6);
			}
		}
	}

	if (instAttribs)
		usage = GL_DYNAMIC_DRAW;

	vao = new VAO(format, usage, instAttribs, maxNrInstances);
	vao->setStaticColor(r, g, b, a);
	vao->uploadMesh(mesh);
}

//---------------------------------------------------------------

void QuadArrayAdj::remove()
{
	vao->remove();
	mesh->remove();
}
}
