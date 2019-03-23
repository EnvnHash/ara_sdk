//
//  Material.h
//  tav_core
//
//  Created by Sven Hahne on 26/8/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <stdio.h>
#include <glm/glm.hpp>

#include "headers/gl_header.h"

namespace tav
{
class Material
{
public:
	Material();
	~Material();

	// set colors
	void setColors(glm::vec4 oDiffuse, glm::vec4 oAmbient, glm::vec4 oSpecular,
			glm::vec4 emissive);
	void setDiffuseColor(glm::vec4 oDiffuse);
	void setAmbientColor(glm::vec4 oAmbient);
	void setSpecularColor(glm::vec4 oSpecular);
	void setEmissiveColor(glm::vec4 oEmmisive);
	void setShininess(float nShininess);

	glm::vec4 getDiffuseColor();
	glm::vec4 getAmbientColor();
	glm::vec4 getSpecularColor();
	glm::vec4 getEmissiveColor();
	float getShininess();

	// apply the material
	virtual void begin();
	virtual void end();

private:
	glm::vec4 diffuse;
	glm::vec4 ambient;
	glm::vec4 specular;
	glm::vec4 emissive;
	float shininess;

	glm::vec4 prev_diffuse, prev_diffuse_back;
	glm::vec4 prev_ambient, prev_ambient_back;
	glm::vec4 prev_specular, prev_specular_back;
	glm::vec4 prev_emissive, prev_emissive_back;
	float prev_shininess, prev_shininess_back;

};
}
