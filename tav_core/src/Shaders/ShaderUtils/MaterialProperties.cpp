//
//  MaterialProperties.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 16.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "Shaders/ShaderUtils/MaterialProperties.h"

namespace tav
{
MaterialProperties::MaterialProperties() :
		ShaderProperties()
{
	init(8);

	parTypes[0].name = "lightColor";
	parTypes[0].type = "4fv";
	parTypes[1].name = "ambient";
	parTypes[1].type = "4fv";
	parTypes[2].name = "diffuse";
	parTypes[2].type = "4fv";
	parTypes[3].name = "lightDirection";
	parTypes[3].type = "3fv";
	parTypes[4].name = "halfVector";
	parTypes[4].type = "3fv";
	parTypes[5].name = "specular";
	parTypes[5].type = "4fv";
	parTypes[6].name = "shininess";
	parTypes[6].type = "f";
	parTypes[7].name = "strength";
	parTypes[7].type = "f";

	setupStd();
}

MaterialProperties::~MaterialProperties()
{
}

void MaterialProperties::setupStd()
{
	glm::vec3 lightDir = glm::normalize(glm::vec3(-0.3f, 0.2f, 1.0f)); // from the object and not to the object

	setColor(1.f, 1.f, 1.f, 1.f);
	setAmbient(0.1f, 0.1f, 0.1f, 1.f);
	setDiffuse(1.f, 1.f, 1.f, 1.f);
	setDirection(lightDir.x, lightDir.y, lightDir.z); // by scaling this, the intensity varies
	setHalfVector(lightDir.x, lightDir.y, lightDir.z);
	setShininess(70.0f);
	setStrength(0.5f);
}

void MaterialProperties::setColor(GLfloat _r, GLfloat _g, GLfloat _b,
		GLfloat _a)
{
	if (!parTypes[0].isSet)
		parTypes[0].isSet = true;
	parTypes[0].f4Val[0] = _r;
	parTypes[0].f4Val[1] = _g;
	parTypes[0].f4Val[2] = _b;
	parTypes[0].f4Val[3] = _a;
}

glm::vec4 MaterialProperties::getColor()
{
	return glm::vec4(parTypes[0].f4Val[0], parTypes[0].f4Val[1],
			parTypes[0].f4Val[2], parTypes[0].f4Val[3]);
}

void MaterialProperties::setEmissive(GLfloat _r, GLfloat _g, GLfloat _b,
		GLfloat _a)
{
	if (!parTypes[0].isSet)
		parTypes[0].isSet = true;
	parTypes[0].f4Val[0] = _r;
	parTypes[0].f4Val[1] = _g;
	parTypes[0].f4Val[2] = _b;
	parTypes[0].f4Val[3] = _a;
}

void MaterialProperties::setAmbient(GLfloat _r, GLfloat _g, GLfloat _b,
		GLfloat _a)
{
	if (!parTypes[1].isSet)
		parTypes[1].isSet = true;
	parTypes[1].f4Val[0] = _r;
	parTypes[1].f4Val[1] = _g;
	parTypes[1].f4Val[2] = _b;
	parTypes[1].f4Val[3] = _a;
}

void MaterialProperties::setDiffuse(GLfloat _r, GLfloat _g, GLfloat _b,
		GLfloat _a)
{
	if (!parTypes[2].isSet)
		parTypes[2].isSet = true;
	parTypes[2].f4Val[0] = _r;
	parTypes[2].f4Val[1] = _g;
	parTypes[2].f4Val[2] = _b;
	parTypes[2].f4Val[3] = _a;
}

glm::vec4 MaterialProperties::getDiffuse()
{
	return glm::vec4(parTypes[2].f4Val[0], parTypes[2].f4Val[1],
			parTypes[2].f4Val[2], parTypes[2].f4Val[3]);
}

void MaterialProperties::setDirection(GLfloat _r, GLfloat _g, GLfloat _b)
{
	if (!parTypes[3].isSet)
		parTypes[3].isSet = true;
	parTypes[3].f3Val[0] = _r;
	parTypes[3].f3Val[1] = _g;
	parTypes[3].f3Val[2] = _b;
}

void MaterialProperties::setHalfVector(GLfloat _r, GLfloat _g, GLfloat _b)
{
	if (!parTypes[4].isSet)
		parTypes[4].isSet = true;
	parTypes[4].f3Val[0] = _r;
	parTypes[4].f3Val[1] = _g;
	parTypes[4].f3Val[2] = _b;
}

void MaterialProperties::setSpecular(GLfloat _r, GLfloat _g, GLfloat _b,
		GLfloat _a)
{
	if (!parTypes[5].isSet)
		parTypes[5].isSet = true;
	parTypes[5].f4Val[0] = _r;
	parTypes[5].f4Val[1] = _g;
	parTypes[5].f4Val[2] = _b;
	parTypes[5].f4Val[3] = _a;
}

void MaterialProperties::setShininess(float _val)
{
	if (!parTypes[6].isSet)
		parTypes[6].isSet = _val;
	parTypes[6].fVal = _val;
}

void MaterialProperties::setStrength(float _val)
{
	if (!parTypes[7].isSet)
		parTypes[7].isSet = _val;
	parTypes[7].fVal = _val;
}
}
