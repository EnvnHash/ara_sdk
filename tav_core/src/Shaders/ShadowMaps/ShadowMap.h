//
//  ShadowMap.h
//  tav_gl4
//
//  Created by Sven Hahne on 18.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

//
//  SHShadow.h
//  tav_gl4
//
//  Created by Sven Hahne on 17.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>

#include "headers/tav_types.h"
#include "GLUtils/GLMCamera.h"
#include "Shaders/Shaders.h"
#include "GLUtils/FBO.h"

namespace tav
{
class ShadowMap
{
public:
	ShadowMap();
	~ShadowMap();
	// virtual void        render(SceneNode* _scene, double time, double dt, unsigned int ctxNr) = 0;
	virtual void setLightPos(glm::vec3 _pos) = 0;

	virtual float* getShadowMatr();
	virtual GLuint getDepthImg();
	virtual GLuint getColorImg();
	virtual FBO* getFbo();
	virtual glm::mat4 getLightProjMatr();
	virtual glm::mat4 lightViewMatr();
	virtual int getWidth();
	virtual int getHeight();
	virtual float getLinearDepthScalar();
	virtual void setLookAtPoint(glm::vec3 _pos);

	Shaders* shadowShader = nullptr;
	camPar* cp;
	//SceneNode*  scene;
	FBO* fbo;
	glm::mat4 n_mvp;
	glm::mat3 n_mat;
	glm::mat4 scene_model_matrix;
	glm::mat4 light_view_matrix;
	glm::mat4 light_projection_matrix;
	glm::mat4 shadowMatr;
	glm::mat4 scale_bias_matrix;

	glm::vec3 light_position;
	glm::vec3 lookAtPoint;

	int nmatLoc = -1;
	int scrWidth;
	int scrHeight;

	float near = 1.f;
	float far = 100.f;
	float shadow_map_coef = 0.5f;
	float linearDepthScalar;
};
}
