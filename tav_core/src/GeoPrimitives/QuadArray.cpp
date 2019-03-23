//
//  QuadArray.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  creates an Array of Quads which will result in one Plane
//  x, y means the lower left corner, nrSegs is per side
//  standard size is 2.f * 2.f from -1|-1|0 to 1|1|0

#include "pch.h"
#include "GeoPrimitives/QuadArray.h"

namespace tav
{
QuadArray::QuadArray(int _nrSegsX, int _nrSegsY, float _x, float _y, float _w,
		float _h, float _r, float _g, float _b, float _a,
		std::vector<coordType>* _instAttribs, int _nrInstances, GLenum _usage) :
		GeoPrimitive(), nrSegsX(_nrSegsX), nrSegsY(_nrSegsY), x(_x), y(_y), totalWidth(
				2.f), totalHeight(2.f), instAttribs(_instAttribs), maxNrInstances(
				_nrInstances), usage(_usage)
{
	qaNormal = glm::vec3(0.f, 0.f, 1.f);
	r = _r;
	g = _g;
	b = _b;
	a = _a;

	init();
}

//---------------------------------------------------------------

QuadArray::QuadArray(int _nrSegsX, int _nrSegsY, float _x, float _y, float _w,
		float _h, glm::vec3 inNormal, std::vector<coordType>* _instAttribs,
		int _nrInstances, GLenum _usage) :
		GeoPrimitive(), nrSegsX(_nrSegsX), nrSegsY(_nrSegsY), x(_x), y(_y), totalWidth(
				2.f), totalHeight(2.f), instAttribs(_instAttribs), maxNrInstances(
				_nrInstances), usage(_usage)
{
	qaNormal = inNormal;
	r = 0.0f;
	g = 0.0f;
	b = 0.0f;
	a = 1.0f;

	init();
}

//---------------------------------------------------------------

QuadArray::~QuadArray()
{
	delete vao;
}

//---------------------------------------------------------------

void QuadArray::init()
{
	GLfloat quadPos[18] =
	{ 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f,
			0.f, 1.f, 0.f };
	GLfloat norm[3] =
	{ 0.f, 0.f, 1.f };

	float quadWidth = totalWidth / static_cast<float>(nrSegsX);
	float quadHeight = totalHeight / static_cast<float>(nrSegsY);

	float texWidth = 1.f / static_cast<float>(nrSegsX);
	float texHeight = 1.f / static_cast<float>(nrSegsY);

	format = "position:3f,normal:3f,texCoord:2f,color:4f";
	mesh = new Mesh(format);

	// move the QuadArray to the correct position, in relation to the dimensions
	for (auto yInd = 0; yInd < nrSegsY; yInd++)
	{
		for (auto xInd = 0; xInd < nrSegsX; xInd++)
		{
			for (auto i = 0; i < 6; i++)
			{
				GLfloat v[3] =
				{ quadPos[i * 3] * quadWidth + quadWidth * xInd + x, quadPos[i
						* 3 + 1] * quadHeight + quadHeight * yInd + y, quadPos[i
						* 3 + 2] };
				mesh->push_back_positions(v, 3);
				mesh->push_back_normals(norm, 3);

				GLfloat t[2] =
				{ quadPos[i * 3] * texWidth + texWidth * xInd,
						quadPos[i * 3 + 1] * texHeight + texHeight * yInd };
				mesh->push_back_texCoords(t, 2);
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

void QuadArray::remove()
{
	vao->remove();
	mesh->remove();
}
}
