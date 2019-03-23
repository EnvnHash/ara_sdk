//
//  MeshPrimitive.h
//  tav_gl4
//
//  Created by Sven Hahne on 29.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include "Meshes/Mesh.h"

namespace tav
{
class MeshPrimitive
{
public:
	MeshPrimitive();
	~MeshPrimitive();

	void scale(float _scaleX, float _scaleY, float _scaleZ);
	void rotate(float _angle, float _rotX, float _rotY, float _rotZ);
	void translate(float _x, float _y, float _z);
	void doTransform(bool transfNormals);

	std::vector<glm::vec3>* getPositions();
	std::vector<glm::vec3>* getNormals();
	std::vector<glm::vec2>* getTexCoords();

	Mesh* getMesh();
	const char* getFormat();

	GLfloat r;
	GLfloat g;
	GLfloat b;
	GLfloat a;

	std::vector<glm::vec3> position;
	std::vector<glm::vec3> normal;
	std::vector<glm::vec2> texCoords;

	Mesh* mesh = nullptr;
	const char* format;

	glm::mat4 transfMatr;
	glm::mat4 normTransfMatr;
};
}
