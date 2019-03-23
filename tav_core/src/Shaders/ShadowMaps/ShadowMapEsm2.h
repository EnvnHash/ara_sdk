//
//  SHShadow.h
//  tav_gl4
//
//  Created by Sven Hahne on 17.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <cmath>
#include "Shaders/ShadowMaps/ShadowMap.h"
#include "GLUtils/GLMCamera.h"
#include "Shaders/Shaders.h"
#include "GLUtils/FBO.h"

namespace tav
{
class ShadowMapEsm2: public ShadowMap
{
public:
	ShadowMapEsm2(GLMCamera* _gCam, int _scrWidth, int _scrHeight,
			glm::vec3 _lightPos, float _near, float _far,
			ShaderCollector* _shCol);
	~ShadowMapEsm2();
	//void        render(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	GLuint getDepthImg();
	GLuint getColorImg();
	glm::mat4 getLightProjMatr();
	glm::mat4 lightViewMatr();
	float* getShadowMatr();
	void setLightPos(glm::vec3 _pos);
	void setLookAtPoint(glm::vec3 _pos);
	int getWidth();
	int getHeight();
	float getCoef();
	FBO* getFbo();
	float getLinearDepthScalar();
	GLenum getColorBufType();
private:
	Shaders* depthShader = nullptr;
	GLMCamera* gCam;
	GLMCamera* lightCam;
	//SceneNode*  scene;
	FBO* fbo;
	glm::mat4 n_mvp;
	glm::mat3 n_mat;
	glm::mat4 light_view_matrix;
	glm::mat4 light_projection_matrix;
	glm::mat4 shadowMatr;
	glm::mat4 scale_bias_matrix;

	glm::vec3 light_position;
	glm::vec3 lookAtPoint;

	int nmatLoc = -1;
	int scrWidth;
	int scrHeight;

	float shadow_map_coef = 0.5f;
	float linearDepthScalar;
	float near;
	float far;

	GLenum colBufType;
};
}
