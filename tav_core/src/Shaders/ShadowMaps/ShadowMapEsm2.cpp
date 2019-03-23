//
//  ShadowMapEsm2.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 12.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Generate a Shadow map for Variance Shadow Mapping
//

#include "pch.h"
#include "ShadowMapEsm2.h"

namespace tav
{
ShadowMapEsm2::ShadowMapEsm2(GLMCamera* _gCam, int _scrWidth, int _scrHeight,
		glm::vec3 _lightPos, float _near, float _far, ShaderCollector* _shCol) :
		ShadowMap(), gCam(_gCam), light_position(_lightPos), near(_near), far(
				_far)
{
	scrWidth =
			static_cast<int>(static_cast<float>(_scrWidth) * shadow_map_coef);
	scrHeight = static_cast<int>(static_cast<float>(_scrHeight)
			* shadow_map_coef);
	colBufType = GL_R32F;

	lookAtPoint = glm::vec3(0.0f, 0.0f, 0.0f);
	shadowMatr = glm::mat4(1.0f);
	scale_bias_matrix = glm::mat4(glm::vec4(0.5f, 0.0f, 0.0f, 0.0f),
			glm::vec4(0.0f, 0.5f, 0.0f, 0.0f),
			glm::vec4(0.0f, 0.0f, 0.5f, 0.0f),
			glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));

	lightCam = new GLMCamera();
	lightCam->setupPerspective(false, 45.0f, near, far, scrWidth, scrHeight);
	lightCam->setCamPos(light_position);
	lightCam->setLookAt(lookAtPoint);
	lightCam->initMatrices(GLMCamera::FRUSTUM);
	lightCam->setModelMatr(gCam->getModelMatr());
	linearDepthScalar = 1.0f / (far - near); // this helps us remap depth values to be linear, problem hier!!!

	const char* vSmapShader =
			{
					"#version 330 core\n"
							"layout(location = 0) in vec4 position;"
							"layout(location = 4) in glm::mat4 modMatr;"
							"uniform int useInstancing;"
							"uniform glm::mat4 model_matrix;"
							"uniform glm::mat4 view_matrix;"
							"uniform glm::mat4 projection_matrix;"
							"out vec4 v_position;"
							"mat4 model_view_matrix;"
							"void main(void) {"
							"model_view_matrix = view_matrix * (useInstancing == 0 ? model_matrix : modMatr);"
							"v_position = model_view_matrix * position;"
							"gl_Position = projection_matrix * (model_view_matrix * position);"
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

	depthShader = new Shaders(vSmapShader, NULL, fSmapShader, false);

	// fbo for saving the depth information
	//fbo = new FBO(scrWidth, scrHeight, FBO::DepthBuf, 0);
	fbo = new FBO(_shCol, scrWidth, scrHeight, colBufType, GL_TEXTURE_2D, false,
			1, 1, 1, GL_REPEAT, false);
}

ShadowMapEsm2::~ShadowMapEsm2()
{
}
/*
 void ShadowMapEsm2::render(SceneNode* _scene, double time, double dt, unsigned int ctxNr)
 {
 // Matrices used when rendering from the light’s position
 depthShader->begin();
 depthShader->setUniform1f("u_LinearDepthConstant", linearDepthScalar);
 lightCam->sendModelM(depthShader->getProgram(), (char*) "model_matrix");
 lightCam->sendViewM(depthShader->getProgram(), (char*) "view_matrix");
 lightCam->sendProjM(depthShader->getProgram(), (char*) "projection_matrix");

 fbo->bind();
 fbo->clearWhite(); // clear on white, since we need depth values

 glEnable(GL_CULL_FACE); // cull front faces - this helps with artifacts and shadows with exponential shadow mapping
 glCullFace(GL_BACK);
 
 // scene needs to be rendered here
 _scene->draw(time, dt, cp, depthShader);
 
 glDisable(GL_CULL_FACE);

 fbo->unbind();
 
 depthShader->end();
 }
 */
GLuint ShadowMapEsm2::getDepthImg()
{
	return fbo->getDepthImg();
}

GLuint ShadowMapEsm2::getColorImg()
{
	return fbo->getColorImg();
}

glm::mat4 ShadowMapEsm2::getLightProjMatr()
{
	return lightCam->getProjectionMatr();
}

glm::mat4 ShadowMapEsm2::lightViewMatr()
{
	return lightCam->getViewMatr();
}

float* ShadowMapEsm2::getShadowMatr()
{
	shadowMatr = scale_bias_matrix * lightCam->getProjectionMatr()
			* lightCam->getViewMatr();
	return &shadowMatr[0][0];
}

// noch schmutzig, könnte noch eleganter sein....
void ShadowMapEsm2::setLightPos(glm::vec3 _pos)
{
	light_position = _pos;
	lightCam->setupPerspective(false, 45.0f, near, far, scrWidth, scrHeight);
	lightCam->setCamPos(light_position);
	lightCam->setLookAt(lookAtPoint);
	lightCam->initMatrices(GLMCamera::FRUSTUM);
	lightCam->setModelMatr(gCam->getModelMatr());
}

void ShadowMapEsm2::setLookAtPoint(glm::vec3 _pos)
{
	lookAtPoint = _pos;
}

int ShadowMapEsm2::getWidth()
{
	return scrWidth;
}

int ShadowMapEsm2::getHeight()
{
	return scrHeight;
}

float ShadowMapEsm2::getCoef()
{
	return shadow_map_coef;
}

FBO* ShadowMapEsm2::getFbo()
{
	return fbo;
}

float ShadowMapEsm2::getLinearDepthScalar()
{
	return linearDepthScalar;
}

GLenum ShadowMapEsm2::getColorBufType()
{
	return colBufType;
}
}
