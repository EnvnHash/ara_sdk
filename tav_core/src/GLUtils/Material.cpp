//
//  Material.cpp
//  tav_core
//
//  Created by Sven Hahne on 26/8/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "Material.h"

namespace tav
{
Material::Material()
{
}

Material::~Material()
{
}

void Material::setColors(glm::vec4 oDiffuse, glm::vec4 oAmbient,
		glm::vec4 oSpecular, glm::vec4 oEmissive)
{
	setDiffuseColor(oDiffuse);
	setAmbientColor(oAmbient);
	setSpecularColor(oSpecular);
	setEmissiveColor(oEmissive);
}

void Material::setDiffuseColor(glm::vec4 oDiffuse)
{
	diffuse = oDiffuse;
}

void Material::setAmbientColor(glm::vec4 oAmbient)
{
	ambient = oAmbient;
}

void Material::setSpecularColor(glm::vec4 oSpecular)
{
	specular = oSpecular;
}

void Material::setEmissiveColor(glm::vec4 oEmissive)
{
	emissive = oEmissive;
}

void Material::setShininess(float nShininess)
{
	shininess = nShininess;
}

float Material::getShininess()
{
	return shininess;
}

glm::vec4 Material::getDiffuseColor()
{
	return diffuse;
}

glm::vec4 Material::getAmbientColor()
{
	return ambient;
}

glm::vec4 Material::getSpecularColor()
{
	return specular;
}

glm::vec4 Material::getEmissiveColor()
{
	return emissive;
}

void Material::begin()
{
#ifndef TARGET_OPENGLES
	// save previous values, opengl es cannot use push/pop attrib
	glGetMaterialfv(GL_FRONT, GL_DIFFUSE, &prev_diffuse.r);
	glGetMaterialfv(GL_FRONT, GL_SPECULAR, &prev_specular.r);
	glGetMaterialfv(GL_FRONT, GL_AMBIENT, &prev_ambient.r);
	glGetMaterialfv(GL_FRONT, GL_EMISSION, &prev_emissive.r);
	glGetMaterialfv(GL_FRONT, GL_SHININESS, &prev_shininess);

	glGetMaterialfv(GL_BACK, GL_DIFFUSE, &prev_diffuse_back.r);
	glGetMaterialfv(GL_BACK, GL_SPECULAR, &prev_specular_back.r);
	glGetMaterialfv(GL_BACK, GL_AMBIENT, &prev_ambient_back.r);
	glGetMaterialfv(GL_BACK, GL_EMISSION, &prev_emissive_back.r);
	glGetMaterialfv(GL_BACK, GL_SHININESS, &prev_shininess_back);

	// Material colors and properties
	glMaterialfv(GL_FRONT, GL_DIFFUSE, &diffuse.r);
	glMaterialfv(GL_FRONT, GL_SPECULAR, &specular.r);
	glMaterialfv(GL_FRONT, GL_AMBIENT, &ambient.r);
	glMaterialfv(GL_FRONT, GL_EMISSION, &emissive.r);
	glMaterialfv(GL_FRONT, GL_SHININESS, &shininess);

	glMaterialfv(GL_BACK, GL_DIFFUSE, &diffuse.r);
	glMaterialfv(GL_BACK, GL_SPECULAR, &specular.r);
	glMaterialfv(GL_BACK, GL_AMBIENT, &ambient.r);
	glMaterialfv(GL_BACK, GL_EMISSION, &emissive.r);
	glMaterialfv(GL_BACK, GL_SHININESS, &shininess);
#else
	// opengl es 1.1 implementation must use GL_FRONT_AND_BACK.

	glGetMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &prev_diffuse.r);
	glGetMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &prev_specular.r);
	glGetMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &prev_ambient.r);
	glGetMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &prev_emissive.r);
	glGetMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &prev_shininess);

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &diffuse.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &specular.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &ambient.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &emissive.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &shininess);
#endif
}

void Material::end()
{
#ifndef TARGET_OPENGLES
	// Set previous material colors and properties
	glMaterialfv(GL_FRONT, GL_DIFFUSE, &prev_diffuse.r);
	glMaterialfv(GL_FRONT, GL_SPECULAR, &prev_specular.r);
	glMaterialfv(GL_FRONT, GL_AMBIENT, &prev_ambient.r);
	glMaterialfv(GL_FRONT, GL_EMISSION, &prev_emissive.r);
	glMaterialfv(GL_FRONT, GL_SHININESS, &prev_shininess);

	glMaterialfv(GL_BACK, GL_DIFFUSE, &prev_diffuse_back.r);
	glMaterialfv(GL_BACK, GL_SPECULAR, &prev_specular_back.r);
	glMaterialfv(GL_BACK, GL_AMBIENT, &prev_ambient_back.r);
	glMaterialfv(GL_BACK, GL_EMISSION, &prev_emissive_back.r);
	glMaterialfv(GL_BACK, GL_SHININESS, &prev_shininess_back);
#else
	// opengl es 1.1 implementation must use GL_FRONT_AND_BACK.

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &prev_diffuse.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &prev_specular.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &prev_ambient.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &prev_emissive.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &prev_shininess);
#endif
}

}
