//
//  ShadowMapEsm.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 12.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Generate a Shadow map for Exponential Shadow Mapping
//

#include "pch.h"
#include "ShadowMapEsm.h"

namespace tav
{
ShadowMapEsm::ShadowMapEsm(camPar* _cp, int _scrWidth, int _scrHeight,
		glm::vec3 _lightPos, float _near, float _far, ShaderCollector* _shCol) :
		ShadowMap()
{
	cp = _cp;
	near = _near;
	far = _far;
	light_position = _lightPos;
	scrWidth =
			static_cast<int>(static_cast<float>(_scrWidth) * shadow_map_coef);
	scrHeight = static_cast<int>(static_cast<float>(_scrHeight)
			* shadow_map_coef);

	colBufType = GL_R32F;
	// fbo for saving the depth information
	fbo = new FBO(_shCol, scrWidth, scrHeight, colBufType, GL_TEXTURE_2D, true,
			1, 1, 1, GL_REPEAT, false);

	light_position = light_position;
	shadowMatr = glm::mat4(1.0f);
	scale_bias_matrix = glm::mat4(glm::vec4(0.5f, 0.0f, 0.0f, 0.0f),
			glm::vec4(0.0f, 0.5f, 0.0f, 0.0f),
			glm::vec4(0.0f, 0.0f, 0.5f, 0.0f),
			glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));

//        lightCam = new GLMCamera();
//        lightCam->setupPerspective(false, 45.0f, near, far, scrWidth, scrHeight);
//        lightCam->setCamPos(light_position);
//        lightCam->setLookAt(lookAtPoint);
//        lightCam->initMatrices(GLMCamera::GLM_FRUSTRUM);
//        lightCam->setModelMatr(cp->model_matrix_mat4);

	linearDepthScalar = 1.0f / (far - near); // this helps us remap depth values to be linear,

	// standard maessig in den nullpunkt schauen
	light_view_matrix = glm::lookAt(light_position, glm::vec3(0, 0, 0),
			glm::vec3(0, 1, 0));
//        light_view_matrix = glm::lookAt(light_position, cp->lookAt, glm::vec3(0,1,0));
	light_projection_matrix = glm::frustum(-1.0f, 1.0f, -1.0f, 1.0f, near, far);

	const char* vSmapShader =
			{
					"#version 330 core\n"
							"layout(location = 0) in vec4 position;"
							"layout(location = 4) in glm::mat4 modMatr;"
							"uniform int useInstancing;"
							"uniform glm::mat4 modelMatrix;"
							"uniform glm::mat4 viewMatrix;"
							"uniform glm::mat4 projectionMatrix;"
							"mat4 model_view_matrix;"
							"out vec4 v_position;"
							"void main(void) {"
							"   model_view_matrix = viewMatrix * (useInstancing == 0 ? modelMatrix : modMatr);"
							"   v_position = model_view_matrix * position;"
							"   gl_Position = projectionMatrix * model_view_matrix * position;"
							"}" };

	const char* fSmapShader =
			{
					"#version 330 core\n"
							"uniform float u_LinearDepthConstant;"
							"in vec4 v_position;"
							"layout (location = 0) out vec4 color;"
							"void main()"
							"{"
							"float linearDepth = length(v_position.xyz) * u_LinearDepthConstant;"
							"color.r = linearDepth;"
							"}" };

	shadowShader = new Shaders(vSmapShader, NULL, fSmapShader, false);
}

//---------------------------------------------------------------

ShadowMapEsm::~ShadowMapEsm()
{
}

//---------------------------------------------------------------

/*
 void ShadowMapEsm::render(SceneNode* _scene, double time, double dt, unsigned int ctxNr)
 {
 // Matrices used when rendering from the light’s position
 shadowShader->begin();
 shadowShader->setUniform1f("u_LinearDepthConstant", linearDepthScalar);
 shadowShader->setUniformMatrix4fv("modelMatrix", cp->model_matrix, 1);
 shadowShader->setUniformMatrix4fv("viewMatrix", &light_view_matrix[0][0], 1);
 shadowShader->setUniformMatrix4fv("projectionMatrix", &light_projection_matrix[0][0], 1);
 
 fbo->bind();
 fbo->clearWhite(); // clear on white, since we need depth values

 glEnable(GL_CULL_FACE); // cull front faces - this helps with artifacts and shadows with exponential shadow mapping
 glCullFace(GL_BACK);
 
 // scene needs to be rendered here
 _scene->draw(time, dt, cp, shadowShader);
 
 glDisable(GL_CULL_FACE);

 fbo->unbind();
 
 shadowShader->end();
 }
 */

//---------------------------------------------------------------
// noch schmutzig, könnte noch eleganter sein....
void ShadowMapEsm::setLightPos(glm::vec3 _pos)
{
	light_position = _pos;
//        lightCam->setupPerspective(false, 45.0f, near, far, scrWidth, scrHeight);
//        lightCam->setCamPos(light_position);
//        lightCam->setLookAt(lookAtPoint);
//        lightCam->initMatrices(GLMCamera::GLM_FRUSTRUM);
//        lightCam->setModelMatr(cp->model_matrix_mat4);
}

//---------------------------------------------------------------

float ShadowMapEsm::getCoef()
{
	return shadow_map_coef;
}

//---------------------------------------------------------------

GLenum ShadowMapEsm::getColorBufType()
{
	return colBufType;
}
}
