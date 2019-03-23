//
//  Torus.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  creates an Array of Quads which will result in one Plane
//  x, y means the lower left corner, nrSegs is per side
//  standard size is 2.f * 2.f from -1|-1|0 to 1|1|0

#include "pch.h"
#include "Torus.h"

namespace tav
{
Torus::Torus() :
		GeoPrimitive()
{
	x = -1.f;
	y = -1.f;
	width = 1.f;
	height = 1.f;
	nrSegsX = 40;
	nrSegsY = 4;

	totalWidth = 2.f;
	totalHeight = 2.f;

	instAttribs = nullptr;
	maxNrInstances = 1;

	qaNormal = glm::vec3(0.f, 0.f, 1.f);
	r = 1.0f;
	g = 1.0f;
	b = 1.0f;
	a = 1.0f;

	init();
}

//---------------------------------------------------------------

Torus::Torus(int _nrSegsX, int _nrSegsY, float rad,
		std::vector<coordType>* _instAttribs, int _nrInstances) :
		GeoPrimitive()
{
	nrSegsX = _nrSegsX;
	nrSegsY = _nrSegsY;

	totalWidth = 2.f;
	totalHeight = 2.f;

	instAttribs = _instAttribs;
	maxNrInstances = _nrInstances;

	qaNormal = glm::vec3(0.f, 0.f, 1.f);

	init();
}

//---------------------------------------------------------------

Torus::~Torus()
{
	delete vao;
	for (int i = 0; i < nrSegsX * nrSegsY; i++)
		delete quads[i];
	delete quads;
}

//---------------------------------------------------------------

void Torus::init()
{
	quads = new MPQuad*[nrSegsX * nrSegsY];
	float quadWidth = totalWidth / static_cast<float>(nrSegsX);
	float quadHeight = totalHeight / static_cast<float>(nrSegsY);
	float xOffs = totalWidth / static_cast<float>(nrSegsX);
	float yOffs = totalHeight / static_cast<float>(nrSegsY);

	for (auto yInd = 0; yInd < nrSegsY; yInd++)
	{
		float yo = static_cast<float>(yInd) * yOffs - (totalWidth * 0.5f);
		for (auto xInd = 0; xInd < nrSegsX; xInd++)
		{
			float xo = static_cast<float>(xInd) * xOffs - (totalHeight * 0.5f);
			quads[yInd * nrSegsX + xInd] = new MPQuad(xo, yo, quadWidth,
					quadHeight, qaNormal);
		}
	}

	format = "position:3f,normal:3f,texCoord:2f,color:4f";
	mesh = new Mesh(format);

	// move the Torus to the correct position, in relation to the dimensions
	for (auto yInd = 0; yInd < nrSegsY; yInd++)
	{
		for (auto xInd = 0; xInd < nrSegsX; xInd++)
		{
			std::vector<glm::vec3>* pos =
					quads[yInd * nrSegsX + xInd]->getPositions();
			std::vector<glm::vec3>* nor =
					quads[yInd * nrSegsX + xInd]->getNormals();

			for (auto i = 0; i < 6; i++)
			{
				GLfloat v[3] =
				{ pos->at(i).x, pos->at(i).y, pos->at(i).z };
				mesh->push_back_positions(v, 3);

				GLfloat n[3] =
				{ nor->at(i).x, nor->at(i).y, nor->at(i).z };
				mesh->push_back_normals(n, 3);

				GLfloat t[2] =
				{ pos->at(i).x / totalWidth + 0.5f, pos->at(i).y / totalHeight
						+ 0.5f };
//                    printf("x: %f y: %f \n", pos->at(i).x, pos->at(i).y);
//                    printf("totalW: %f totalH: %f \n", totalWidth, totalHeight);
//                    printf("%f %f \n", pos->at(i).x / totalWidth + 0.5f, pos->at(i).y / totalHeight + 0.5f);
//                    printf("\n");

				mesh->push_back_texCoords(t, 2);
			}
			quads[yInd * nrSegsX + xInd]->remove();
		}
	}

	GLenum usage = GL_DYNAMIC_DRAW;
	if (instAttribs)
		usage = GL_DYNAMIC_DRAW;

	vao = new VAO(format, usage, instAttribs, maxNrInstances);
	vao->setStaticColor(r, g, b, a);
	vao->uploadMesh(mesh);
}

//---------------------------------------------------------------

void Torus::remove()
{
	vao->remove();
	mesh->remove();
}
}
