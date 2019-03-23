//
//  LightProperties.h
//  tav_gl4
//
//  Created by Sven Hahne on 16.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include "ShaderProperties.h"
#include "../../GLUtils/glm_utils.h"

namespace tav
{
class LightProperties: public ShaderProperties
{
public:
	LightProperties();
	virtual ~LightProperties();

	enum lparType
	{
		Ambient = 0,
		Color = 1,
		LPos = 2,
		Dir = 3,
		halfVec = 4,
		ConeDir = 5,
		EyeDir = 6,
		spotCut = 7,
		spotExp = 8,
		constAtt = 9,
		linAtt = 10,
		quadAtt = 11,
		shine = 12,
		strength = 13,
		scaleFact = 14,
		isSpt = 15,
		isLoc = 16,
		enabl = 17,
		Specu = 18,
		numLPar = 19
	};
	void enable(bool _b);
	void setUpStd();

	void isLocal(bool _b);
	void isSpot(bool _b);
	void setAmbient(GLfloat _r, GLfloat _g, GLfloat _b);
	void setColor(GLfloat _r, GLfloat _g, GLfloat _b);
	void setSpecular(GLfloat _r, GLfloat _g, GLfloat _b);
	void setPosition(GLfloat _x, GLfloat _y, GLfloat _z);
	void setDirection(GLfloat _x, GLfloat _y, GLfloat _z);
	void setHalfVector(GLfloat _x, GLfloat _y, GLfloat _z);
	void setConeDirection(GLfloat _x, GLfloat _y, GLfloat _z);
	void setEyeDirection(GLfloat _x, GLfloat _y, GLfloat _z);
	void setSpotCosCutoff(GLfloat _val);
	void setSpotExponent(GLfloat _val);
	void setConstantAttenuation(GLfloat _val);
	void setLinearAttenuation(GLfloat _val);
	void setQuadraticAttenuation(GLfloat _val);
	void setStrength(GLfloat _strength);
	void setShininess(GLfloat _shine);
	void setScaleFactor(GLfloat _val);

	glm::vec3 getLightPosition();
	GLfloat* getLightPosFv();
};
}
