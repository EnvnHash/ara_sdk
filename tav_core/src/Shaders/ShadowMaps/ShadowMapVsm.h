//
//  SHShadow.h
//  tav_gl4
//
//  Created by Sven Hahne on 17.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include "GLUtils/GLMCamera.h"
#include "Shaders/Shaders.h"
#include "GLUtils/FBO.h"
#include "Shaders/ShadowMaps/ShadowMap.h"

namespace tav
{
class ShadowMapVsm: public ShadowMap
{
public:
	ShadowMapVsm(GLMCamera* _gCam, int _scrWidth, int _scrHeight,
			ShaderCollector* _shCol);
	~ShadowMapVsm();
	// void        render(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	GLuint getDepthImg();
	GLuint getColorImg();
	glm::mat4 getLightProjMatr();
	glm::mat4 lightViewMatr();
	void setLightPos(glm::vec3 _pos);
	void setLookAtPoint(glm::vec3 _pos);
	int getWidth();
	int getHeight();
	float getCoef();
	FBO* getFbo();

private:
	Shaders* shadowShader = nullptr;
	GLMCamera* gCam;
	//SceneNode*  scene;
	FBO* fbo;
	glm::mat4 n_mvp;
	glm::mat3 n_mat;
	glm::mat4 scene_model_matrix;
	glm::mat4 light_view_matrix;
	glm::mat4 light_projection_matrix;

	glm::vec3 light_position;
	glm::vec3 lookAtPoint;

	int scrWidth;
	int scrHeight;

	float shadow_map_coef = 1.0f;
};
}
