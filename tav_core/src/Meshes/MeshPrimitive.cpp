//
//  MeshPrimitive.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 29.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "MeshPrimitive.h"

namespace tav
{
MeshPrimitive::MeshPrimitive()
{
}

MeshPrimitive::~MeshPrimitive()
{
}

//---------------------------------------------------------------

void MeshPrimitive::scale(float _scaleX, float _scaleY, float _scaleZ)
{
	transfMatr = glm::scale(glm::vec3(_scaleX, _scaleY, _scaleZ));
	doTransform(false);
}

//---------------------------------------------------------------

void MeshPrimitive::rotate(float _angle, float _rotX, float _rotY, float _rotZ)
{
	transfMatr = glm::rotate(_angle, glm::vec3(_rotX, _rotY, _rotZ));
	normTransfMatr = glm::rotate(_angle, glm::vec3(_rotX, _rotY, _rotZ));
	doTransform(true);
}

//---------------------------------------------------------------

void MeshPrimitive::translate(float _x, float _y, float _z)
{
	transfMatr = glm::translate(glm::vec3(_x, _y, _z));
	doTransform(false);
}

//---------------------------------------------------------------

void MeshPrimitive::doTransform(bool transfNormals)
{
	for (std::vector<glm::vec3>::iterator it = position.begin();
			it != position.end(); ++it)
	{
		glm::vec4 res = transfMatr * glm::vec4((*it).x, (*it).y, (*it).z, 1.0f);
		(*it).x = res.x;
		(*it).y = res.y;
		(*it).z = res.z;
	}

	if (static_cast<int>(normal.size()) != 0 && transfNormals)
	{
		for (std::vector<glm::vec3>::iterator it = normal.begin();
				it != normal.end(); ++it)
		{
			glm::vec4 res = transfMatr
					* glm::vec4((*it).x, (*it).y, (*it).z, 1.0f);
			(*it).x = res.x;
			(*it).y = res.y;
			(*it).z = res.z;
		}
	}
}

//---------------------------------------------------------------

std::vector<glm::vec3>* MeshPrimitive::getPositions()
{
	return &position;
}

//---------------------------------------------------------------

std::vector<glm::vec3>* MeshPrimitive::getNormals()
{
	return &normal;
}

//---------------------------------------------------------------

std::vector<glm::vec2>* MeshPrimitive::getTexCoords()
{
	return &texCoords;
}

//---------------------------------------------------------------

Mesh* MeshPrimitive::getMesh()
{
	return mesh;
}

//---------------------------------------------------------------

const char* MeshPrimitive::getFormat()
{
	return format;
}

}
