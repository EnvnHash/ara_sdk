//
//  Torus.h
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
class Torus: public GeoPrimitive
{
public:
	Torus();
	Torus(int _nrSegsX, int _nrSegsY, float rad,
			std::vector<coordType>* _instAttribs = nullptr,
			int _nrInstances = 1);
	~Torus();
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
	MPQuad** quads;
	glm::vec3 qaNormal;

	std::vector<coordType>* instAttribs;
	int maxNrInstances = 1;
};
}
