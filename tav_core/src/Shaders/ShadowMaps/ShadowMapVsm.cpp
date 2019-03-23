//
//  ShadowMapVsm.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 12.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Generate a Shadow map for Variance Shadow Mapping
//

#include "pch.h"
#include "ShadowMapVsm.h"

namespace tav
{
ShadowMapVsm::ShadowMapVsm(GLMCamera* _gCam, int _scrWidth, int _scrHeight,
		ShaderCollector* _shCol) :
		ShadowMap()
{
	gCam = _gCam;

	scrWidth =
			static_cast<int>(static_cast<float>(_scrWidth) * shadow_map_coef);
	scrHeight = static_cast<int>(static_cast<float>(_scrHeight)
			* shadow_map_coef);

	const char* vSmapShader =
			{
					"#version 330 core\n"
							"layout(location = 0) in vec4 position;"
							"layout(location = 4) in glm::mat4 modMatr;"
							"uniform int useInstancing;"
							"uniform glm::mat4 model_matrix;"
							"uniform glm::mat4 view_matrix;"
							"uniform glm::mat4 projection_matrix;"
							"mat4 model_view_matrix;"
							"out vec4 v_position;"
							"void main(void) {"
							"   model_view_matrix = view_matrix * (useInstancing == 0 ? model_matrix : modMatr);"
							"   gl_Position = projection_matrix * (model_view_matrix * position);"
							"   v_position = gl_Position;"
							"}" };

	const char* fSmapShader =
	{ "#version 330 core\n"
			"in vec4 v_position;"
			"layout (location = 0) out vec4 color;"
			"void main()"
			"{"
			"float depth = v_position.z / v_position.w ;"
			//Don't forget to move away from unit cube ([-1,1]) to [0,1] coordinate system
			"depth = depth * 0.5 + 0.5;"
			"float moment1 = depth;"
			// Adjusting moments (this is sort of bias per pixel) using derivative
			"float dx = dFdx(depth);"
			"float dy = dFdy(depth);"
			"float moment2 = depth * depth + 0.25*(dx*dx+dy*dy);"
			"color = vec4( moment1,moment2, 0.0, 0.0 );"
			"}" };

	shadowShader = new Shaders(vSmapShader, NULL, fSmapShader, false);

	// fbo for saving the depth information
	fbo = new FBO(_shCol, scrWidth, scrHeight, GL_RGB16F, GL_TEXTURE_2D, true,
			1, 1, 1, GL_REPEAT, false);

	light_position = glm::vec3(-0.5f, 1.0f, 2.0f);
	lookAtPoint = glm::vec3(0.0f, 0.0f, 0.0f);
}

ShadowMapVsm::~ShadowMapVsm()
{
}
/*
 void ShadowMapVsm::render(SceneNode* _scene, double time, double dt, unsigned int ctxNr)
 {        
 // Matrices for rendering the scene
 scene_model_matrix = gCam->getModelMatr();
 
 // Matrices used when rendering from the light’s position
 light_view_matrix = glm::lookAt(light_position, lookAtPoint, glm::vec3(0,1,0));
 light_projection_matrix = glm::frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10.0f);
 //n_mvp = light_projection_matrix * light_view_matrix * scene_model_matrix;
 
 shadowShader->begin();        
 //shadowShader->setUniformMatrix4fv("m_pvm", &n_mvp[0][0], 1);
 shadowShader->setUniformMatrix4fv("model_matrix", &scene_model_matrix[0][0], 1);
 shadowShader->setUniformMatrix4fv("view_matrix", &light_view_matrix[0][0], 1);
 shadowShader->setUniformMatrix4fv("projection_matrix", &light_projection_matrix[0][0], 1);
 
 fbo->bind();
 fbo->clearWhite();   // fbo must clear on white, for rendering depth values

 glEnable(GL_CULL_FACE);
 glCullFace(GL_BACK);    // render only backface, for avoiding self shadowing
 
 // scene needs to be rendered here
 _scene->draw(time, dt, cp, shadowShader);
 
 glDisable(GL_CULL_FACE);
 
 fbo->unbind();
 shadowShader->end();
 }
 */

GLuint ShadowMapVsm::getDepthImg()
{
	return fbo->getDepthImg();
}

GLuint ShadowMapVsm::getColorImg()
{
	return fbo->getColorImg();
}

glm::mat4 ShadowMapVsm::getLightProjMatr()
{
	return light_projection_matrix;
}

glm::mat4 ShadowMapVsm::lightViewMatr()
{
	return light_view_matrix;
}

void ShadowMapVsm::setLightPos(glm::vec3 _pos)
{
	light_position = _pos;
}

void ShadowMapVsm::setLookAtPoint(glm::vec3 _pos)
{
	lookAtPoint = _pos;
}

int ShadowMapVsm::getWidth()
{
	return scrWidth;
}

int ShadowMapVsm::getHeight()
{
	return scrHeight;
}

float ShadowMapVsm::getCoef()
{
	return shadow_map_coef;
}

FBO* ShadowMapVsm::getFbo()
{
	return fbo;
}
}
