//
//  QuadArrayAdj.h
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//
//

#pragma once

#include <iostream>
#include <vector>
#include "GeoPrimitives/GeoPrimitive.h"
#include "../Meshes/MPQuad.h"
#include "GLUtils/glm_utils.h"

namespace tav
{
class QuadArrayAdj: public GeoPrimitive
{
public:
	QuadArrayAdj();
	QuadArrayAdj(int _nrSegsX, int _nrSegsY, float _x = -1.f, float _y = -1.f,
			float _w = 2.f, float _h = 2.f, float _r = 1.f, float _g = 1.f,
			float _b = 1.f, float _a = 1.f,
			std::vector<coordType>* _instAttribs = nullptr,
			int _nrInstances = 1, GLenum _usage = GL_STATIC_DRAW);
	QuadArrayAdj(int _nrSegsX, int _nrSegsY, float _x, float _y, float _w,
			float _h, glm::vec3 inNormal, std::vector<coordType>* _instAttribs =
					nullptr, int _nrInstances = 1, GLenum _usage =
					GL_STATIC_DRAW);
	~QuadArrayAdj();
	void init();
	void remove();
private:
	int nrSegsX;
	int nrSegsY;
	float x;
	float y;
	float width;
	float height;
	float totalWidth;
	float totalHeight;

	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texCoords;
	glm::vec3 qaNormal;

	std::vector<coordType>* instAttribs;
	int maxNrInstances = 1;

	GLenum usage;
};
}
