//
//  Cube.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  basic Cube with straight normals
//  size is (2|2|2), position is (0|0|0)
//
//  subdivision is meant in relation to a single edge
//  e.g. it´s always ^2
//
//  each side is drawn as triangles => fastest way
// 

#include "pch.h"
#include "GeoPrimitives/Cube.h"

namespace tav
{
Cube::Cube(float _width, float _height, float _depth, int _nrSubDiv,
		std::vector<coordType>* _instAttribs, int _maxNrInstances, float _r,
		float _g, float _b, float _a, bool _invNormals, bool _smoothNor) :
		GeoPrimitive(), width(_width), height(_height), depth(_depth), nrSubDiv(
				1), invNormals(_invNormals), smoothNor(_smoothNor), instAttribs(
				nullptr), maxNrInstances(1)
{
	r = _r;
	g = _g;
	b = _b;
	a = _a;

	init();
}

//----------------------------------------------------------------------------------------

Cube::Cube(std::vector<coordType>* _instAttribs, int _maxNrInstances, float _r,
		float _g, float _b, float _a) :
		GeoPrimitive(), width(2.f), height(2.f), depth(2.f), nrSubDiv(1), invNormals(
				false), smoothNor(true), instAttribs(_instAttribs), maxNrInstances(
				_maxNrInstances)
{
	r = _r;
	g = _g;
	b = _b;
	a = _a;

	init();
}

//----------------------------------------------------------------------------------------

Cube::~Cube()
{
}

void Cube::init()
{
	format = "position:3f,normal:3f,texCoord:2f,color:4f";
	mesh = new Mesh(format);

	// Cube sides
	sides.push_back(std::make_pair(glm::vec3(0.f, 0.f, 1.f), depth * 0.5)); // front
	sides.push_back(std::make_pair(glm::vec3(1.f, 0.f, 0.f), width * 0.5)); // right
	sides.push_back(std::make_pair(glm::vec3(0.f, 0.f, -1.f), depth * 0.5)); // back
	sides.push_back(std::make_pair(glm::vec3(-1.f, 0.f, 0.f), width * 0.5)); // left
	sides.push_back(std::make_pair(glm::vec3(0.f, 1.f, 0.f), height * 0.5)); // top
	sides.push_back(std::make_pair(glm::vec3(0.f, -1.f, 0.f), height * 0.5)); // bottom

	// invert normals
	if (invNormals)
		for (std::vector<std::pair<glm::vec3, float>>::iterator it =
				sides.begin(); it != sides.end(); ++it)
			(*it).first = glm::vec3((*it).first.x * -1.f, (*it).first.y * -1.f,
					(*it).first.z * -1.f);

	float quadWidth = width / static_cast<float>(nrSubDiv);
	float quadHeight = height / static_cast<float>(nrSubDiv);

	// take nrSubDiv^2 quads for each side and transform them via the sides
	for (std::vector<std::pair<glm::vec3, float> >::iterator it = sides.begin();
			it != sides.end(); ++it)
	{
		for (auto y = 0; y < nrSubDiv; y++)
		{
			for (auto x = 0; x < nrSubDiv; x++)
			{
				MPQuad q = MPQuad(
						(width * -0.5f) + quadWidth * static_cast<float>(x),
						(height * -0.5f) + quadHeight * static_cast<float>(y),
						quadWidth, quadHeight, (*it).first);

				// write positions and normals
				std::vector<glm::vec3>* pos = q.getPositions();
				std::vector<glm::vec2>* texCoords = q.getTexCoords();

				int ind = 0;
				for (std::vector<glm::vec3>::iterator pIt = pos->begin();
						pIt != pos->end(); ++pIt)
				{
					// move the quad to the correct position, in relation to the dimensions
					glm::vec3 tPos = (*pIt) + (*it).first * (*it).second;
					GLfloat v[3] = { tPos.x, tPos.y, tPos.z };
					mesh->push_back_positions(v, 3);

					if (smoothNor)
					{
						glm::vec3 dirToZero = glm::normalize(tPos);
						GLfloat n[3] = { dirToZero.x, dirToZero.y, dirToZero.z };
						mesh->push_back_normals(n, 3);
					}
					else
					{
						GLfloat n[3] = { (*it).first.x, (*it).first.y, (*it).first.z };
						mesh->push_back_normals(n, 3);
					}

					GLfloat t[2] = { texCoords->at(ind).x / static_cast<float>(nrSubDiv)
							+ static_cast<float>(x)
									/ static_cast<float>(nrSubDiv),
							texCoords->at(ind).y / static_cast<float>(nrSubDiv)
									+ static_cast<float>(y)
											/ static_cast<float>(nrSubDiv) };
					mesh->push_back_texCoords(t, 2);

					GLfloat c[4] = { r, g, b, a };
					mesh->push_back_colors(c, 4);

					ind++;
				}
				//q.remove();
			}
		}
	}

	GLenum usage = GL_STATIC_DRAW;
	if (instAttribs)
		usage = GL_DYNAMIC_DRAW;

	vao = new VAO(format, usage, instAttribs, maxNrInstances);
	vao->uploadMesh(mesh);
}

void Cube::remove()
{
	vao->remove();
	mesh->remove();
}

}
