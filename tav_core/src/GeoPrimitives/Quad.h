//
//  Quad.h
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
#include "GLUtils/glm_utils.h"

namespace tav
{
class Quad: public GeoPrimitive
{
public:
	Quad();
	Quad(float x, float y, float w, float h,
			glm::vec3 inNormal = glm::vec3(0.f, 0.f, 1.f), float _r = 1.f,
			float _g = 1.f, float _b = 1.f, float _a = 1.f,
			std::vector<coordType>* _instAttribs = nullptr,
			int _nrInstances = 1, bool _flipHori = false);
	~Quad();
	void init();
	void remove();
	std::vector<glm::vec3>* getPositions();
	std::vector<glm::vec3>* getNormals();
	std::vector<glm::vec2>* getTexCoords();
private:
	float width;
	float height;
	std::vector<glm::vec3> position;
	std::vector<glm::vec3> normal;
	std::vector<glm::vec2> texCoords;

	std::vector<coordType>* instAttribs;
	int maxNrInstances = 1;
};
}
