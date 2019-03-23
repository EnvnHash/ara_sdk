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
#include "ShadowMapStd.h"

namespace tav
{

ShadowMapStd::ShadowMapStd(camPar* _cp, int _scrWidth, int _scrHeight,
		glm::vec3 _lightPos, float _near, float _far, ShaderCollector* _shCol) :
		ShadowMap()
{
	cp = _cp;
	near = _near;
	far = _far;
	light_position = _lightPos;
	// shadow_map_coef = 0.25f;
	scrWidth = _scrWidth * shadow_map_coef;
	scrHeight = _scrHeight * shadow_map_coef;

	// fbo for saving the depth information
	fbo = new FBO(_shCol, scrWidth, scrHeight, GL_DEPTH32F_STENCIL8,
			GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false);

	light_position = light_position;
	shadowMatr = glm::mat4(1.0f);
	scale_bias_matrix = glm::mat4(glm::vec4(0.5f, 0.0f, 0.0f, 0.0f),
			glm::vec4(0.0f, 0.5f, 0.0f, 0.0f),
			glm::vec4(0.0f, 0.0f, 0.5f, 0.0f),
			glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
	linearDepthScalar = 1.0f / (far - near); // this helps us remap depth values to be linear

	// standard maessig in den nullpunkt schauen
	light_view_matrix = glm::lookAt(light_position, glm::vec3(0, 0, 0),
			glm::vec3(0, 1, 0));
	//light_view_matrix = glm::lookAt(light_position, cp->lookAt, glm::vec3(0,1,0));
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
							"void main(void) {"
							"model_view_matrix = viewMatrix * (useInstancing == 0 ? modelMatrix : modMatr);"
							"gl_Position = projectionMatrix * (model_view_matrix * position);"
							"}" };

	const char* fSmapShader =
	{
			"#version 330 core\n"
			"layout (location = 0) out vec4 color;"
			"void main(void) {"
			"    color = vec4(1.0);"
			"}"
	};

	shadowShader = new Shaders(vSmapShader, NULL, fSmapShader, false);
}

//---------------------------------------------------------------

ShadowMapStd::~ShadowMapStd()
{
}

//---------------------------------------------------------------


/*
 void ShadowMapStd::render(SceneNode* _scene, double time, double dt, unsigned int ctxNr)
 {
 shadowShader->begin();
 shadowShader->setUniformMatrix4fv("modelMatrix", cp->model_matrix, 1);
 shadowShader->setUniformMatrix4fv("viewMatrix", &light_view_matrix[0][0], 1);
 shadowShader->setUniformMatrix4fv("projectionMatrix", &light_projection_matrix[0][0], 1);
 
 fbo->bind();
 fbo->clear();
 glEnable(GL_POLYGON_OFFSET_FILL);
 glPolygonOffset(0.0001f, 0.0001f);

 glEnable(GL_CULL_FACE);
 glCullFace(GL_BACK);    // render only backface, for avoiding self shadowing

 // scene needs to be rendered here
 _scene->draw(time, dt, cp, shadowShader);

 glDisable(GL_CULL_FACE);
 glDisable(GL_POLYGON_OFFSET_FILL);
 fbo->unbind();
 shadowShader->end();
 }
 */

//---------------------------------------------------------------

void ShadowMapStd::setLightPos(glm::vec3 _pos)
{
	light_position = _pos;
}

}
