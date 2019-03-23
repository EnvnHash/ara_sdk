//
//  ShadowMapStd.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 12.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Standard directional light
//

#include "pch.h"
#include "Shaders/ShadowMaps/ShadowMap.h"

namespace tav
{
ShadowMap::ShadowMap()
{
}

//---------------------------------------------------------------

ShadowMap::~ShadowMap()
{
}

//---------------------------------------------------------------

float* ShadowMap::getShadowMatr()
{
	// std::cout << "scale: " << glm::to_string(scale_bias_matrix) << std::endl;
	shadowMatr = scale_bias_matrix * light_projection_matrix
			* light_view_matrix;
	return &shadowMatr[0][0];
}

//---------------------------------------------------------------

GLuint ShadowMap::getDepthImg()
{
	return fbo->getDepthImg();
}

//---------------------------------------------------------------

GLuint ShadowMap::getColorImg()
{
	return fbo->getColorImg();
}

//---------------------------------------------------------------

FBO* ShadowMap::getFbo()
{
	return fbo;
}

//---------------------------------------------------------------

glm::mat4 ShadowMap::getLightProjMatr()
{
	return light_projection_matrix;
}

//---------------------------------------------------------------

glm::mat4 ShadowMap::lightViewMatr()
{
	return light_view_matrix;
}

//---------------------------------------------------------------

int ShadowMap::getWidth()
{
	return scrWidth;
}

//---------------------------------------------------------------

int ShadowMap::getHeight()
{
	return scrHeight;
}

//---------------------------------------------------------------

float ShadowMap::getLinearDepthScalar()
{
	return linearDepthScalar;
}

//---------------------------------------------------------------

void ShadowMap::setLookAtPoint(glm::vec3 _pos)
{
	lookAtPoint = _pos;
}
}
