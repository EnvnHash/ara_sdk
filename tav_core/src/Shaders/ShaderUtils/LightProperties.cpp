//
//  LightProperties.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 16.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  only sends what is set manually!!!

#include "pch.h"
#include "LightProperties.h"

namespace tav
{
LightProperties::LightProperties() :
		ShaderProperties()
{
	init(numLPar);

	// true to apply this light in this invocation
	parTypes[enabl].name = "isEnabled";
	parTypes[enabl].type = "b";
	// true for a point light or a spotlight,  false for a positional light
	parTypes[isLoc].name = "isLocal";
	parTypes[isLoc].type = "b";
	// true if the light is a spotlight
	parTypes[isSpt].name = "isSpot";
	parTypes[isSpt].type = "b";
	// light’s contribution to ambient light
	parTypes[Ambient].name = "ambient";
	parTypes[Ambient].type = "3fv";
	// color of light
	parTypes[Color].name = "LColor";
	parTypes[Color].type = "3fv";

	parTypes[Specu].name = "LSpecular";
	parTypes[Specu].type = "3fv";

	// location of light, if is Local is true, otherwise the direction toward the light
	parTypes[LPos].name = "LPosition";
	parTypes[LPos].type = "3fv";
	// direction of highlights for directional light
	parTypes[Dir].name = "LDirection";
	parTypes[Dir].type = "3fv";
	// direction of highlights for directional light
	parTypes[halfVec].name = "halfVector";
	parTypes[halfVec].type = "3fv";
	// spotlight attributes
	parTypes[ConeDir].name = "coneDirection";
	parTypes[ConeDir].type = "3fv";
	parTypes[EyeDir].name = "eyeDirection";
	parTypes[EyeDir].type = "3fv";
	parTypes[spotCut].name = "spotCosCutoff";
	parTypes[spotCut].type = "f";
	parTypes[spotExp].name = "spotExponent";
	parTypes[spotExp].type = "f";
	parTypes[constAtt].name = "constantAttenuation";
	parTypes[constAtt].type = "f";
	parTypes[linAtt].name = "linearAttenuation";
	parTypes[linAtt].type = "f";
	parTypes[quadAtt].name = "quadraticAttenuation";
	parTypes[quadAtt].type = "f";
	parTypes[shine].name = "shininess";
	parTypes[shine].type = "f";
	parTypes[strength].name = "strength";
	parTypes[strength].type = "f";
	parTypes[scaleFact].name = "ScaleFactor";
	parTypes[scaleFact].type = "f";
}

LightProperties::~LightProperties()
{
}

//---------------------------------------------------------------------

void LightProperties::enable(bool _b)
{
	if (!parTypes[enabl].isSet)
		parTypes[enabl].isSet = true;
	parTypes[enabl].bVal = _b;
}

void LightProperties::isLocal(bool _b)
{
	if (!parTypes[isLoc].isSet)
		parTypes[isLoc].isSet = true;
	parTypes[isLoc].bVal = _b;
}

void LightProperties::isSpot(bool _b)
{
	if (!parTypes[isSpt].isSet)
		parTypes[isSpt].isSet = true;
	parTypes[isSpt].bVal = _b;
}

void LightProperties::setAmbient(GLfloat _r, GLfloat _g, GLfloat _b)
{
	if (!parTypes[Ambient].isSet)
		parTypes[Ambient].isSet = true;
	parTypes[Ambient].f3Val[0] = _r;
	parTypes[Ambient].f3Val[1] = _g;
	parTypes[Ambient].f3Val[2] = _b;
}

void LightProperties::setColor(GLfloat _r, GLfloat _g, GLfloat _b)
{
	if (!parTypes[Color].isSet)
		parTypes[Color].isSet = true;
	parTypes[Color].f3Val[0] = _r;
	parTypes[Color].f3Val[1] = _g;
	parTypes[Color].f3Val[2] = _b;
}

void LightProperties::setSpecular(GLfloat _r, GLfloat _g, GLfloat _b)
{
	if (!parTypes[Specu].isSet)
		parTypes[Specu].isSet = true;
	parTypes[Specu].f3Val[0] = _r;
	parTypes[Specu].f3Val[1] = _g;
	parTypes[Specu].f3Val[2] = _b;
}

void LightProperties::setPosition(GLfloat _x, GLfloat _y, GLfloat _z)
{
	if (!parTypes[LPos].isSet)
		parTypes[LPos].isSet = true;
	parTypes[LPos].f3Val[0] = _x;
	parTypes[LPos].f3Val[1] = _y;
	parTypes[LPos].f3Val[2] = _z;
}

void LightProperties::setDirection(GLfloat _x, GLfloat _y, GLfloat _z)
{
	if (!parTypes[Dir].isSet)
		parTypes[Dir].isSet = true;
	parTypes[Dir].f3Val[0] = _x;
	parTypes[Dir].f3Val[1] = _y;
	parTypes[Dir].f3Val[2] = _z;
}

void LightProperties::setHalfVector(GLfloat _x, GLfloat _y, GLfloat _z)
{
	if (!parTypes[halfVec].isSet)
		parTypes[halfVec].isSet = true;
	parTypes[halfVec].f3Val[0] = _x;
	parTypes[halfVec].f3Val[1] = _y;
	parTypes[halfVec].f3Val[2] = _z;
}

void LightProperties::setConeDirection(GLfloat _x, GLfloat _y, GLfloat _z)
{
	if (!parTypes[ConeDir].isSet)
		parTypes[ConeDir].isSet = true;
	parTypes[ConeDir].f3Val[0] = _x;
	parTypes[ConeDir].f3Val[1] = _y;
	parTypes[ConeDir].f3Val[2] = _z;
}

void LightProperties::setEyeDirection(GLfloat _x, GLfloat _y, GLfloat _z)
{
	if (!parTypes[EyeDir].isSet)
		parTypes[EyeDir].isSet = true;
	parTypes[EyeDir].f3Val[0] = _x;
	parTypes[EyeDir].f3Val[1] = _y;
	parTypes[EyeDir].f3Val[2] = _z;
}

void LightProperties::setSpotCosCutoff(GLfloat _val)
{
	if (!parTypes[spotCut].isSet)
		parTypes[spotCut].isSet = true;
	parTypes[spotCut].fVal = _val;
}

void LightProperties::setSpotExponent(GLfloat _val)
{
	if (!parTypes[spotExp].isSet)
		parTypes[spotExp].isSet = true;
	parTypes[spotExp].fVal = _val;
}

void LightProperties::setConstantAttenuation(GLfloat _val)
{
	if (!parTypes[constAtt].isSet)
		parTypes[constAtt].isSet = true;
	parTypes[constAtt].fVal = _val;
}

void LightProperties::setLinearAttenuation(GLfloat _val)
{
	if (!parTypes[linAtt].isSet)
		parTypes[linAtt].isSet = true;
	parTypes[linAtt].fVal = _val;
}

void LightProperties::setQuadraticAttenuation(GLfloat _val)
{
	if (!parTypes[quadAtt].isSet)
		parTypes[quadAtt].isSet = true;
	parTypes[quadAtt].fVal = _val;
}

void LightProperties::setShininess(GLfloat _val)
{
	if (!parTypes[shine].isSet)
		parTypes[shine].isSet = true;
	parTypes[shine].fVal = _val;
}

void LightProperties::setStrength(GLfloat _val)
{
	if (!parTypes[strength].isSet)
		parTypes[strength].isSet = true;
	parTypes[strength].fVal = _val;
}

void LightProperties::setScaleFactor(GLfloat _val)
{
	if (!parTypes[scaleFact].isSet)
		parTypes[scaleFact].isSet = true;
	parTypes[scaleFact].fVal = _val;
}

// -----------------------------------------------------------

glm::vec3 LightProperties::getLightPosition()
{
	return glm::vec3(parTypes[LPos].f3Val[0], parTypes[LPos].f3Val[1],
			parTypes[LPos].f3Val[2]);
}

GLfloat* LightProperties::getLightPosFv()
{
	return &parTypes[LPos].f3Val[0];
}
}
