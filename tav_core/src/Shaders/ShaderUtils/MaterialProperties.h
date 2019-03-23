//
//  MaterialProperties.h
//  tav_gl4
//
//  Created by Sven Hahne on 16.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include "ShaderProperties.h"

namespace tav
{
class MaterialProperties: public ShaderProperties
{
public:
	MaterialProperties();
	~MaterialProperties();

	void setupStd();

	void setColor(GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a);
	void setEmissive(GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a);
	void setAmbient(GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a);
	void setDiffuse(GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a);
	void setDirection(GLfloat _r, GLfloat _g, GLfloat _b);
	void setHalfVector(GLfloat _r, GLfloat _g, GLfloat _b);
	void setSpecular(GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a);
	void setShininess(float _val);
	void setStrength(float _val);

	glm::vec4 getColor();
	glm::vec4 getDiffuse();

};
}
