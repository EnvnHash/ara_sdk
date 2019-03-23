//
//  Cs3InCubeScaler.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Dreckiges Mapping ohne Light Prototyping
//  für full hd 1920 x 1080
//  input must be all triangles
//

#include "pch.h"
#include "Cs3InCubeScaler.h"

namespace tav
{
Cs3InCubeScaler::Cs3InCubeScaler(sceneData* _scd, OSCData* _osc) :
		CameraSet(1, _scd, _osc)
{
	id = _scd->id;
	nrCameras = 3;

	cam = new GLMCamera*[nrCameras];

	lookAt[0] = glm::vec3(0.f, 0.f, 0.f);
	upVec[0] = glm::vec3(0.f, 1.f, 0.f);
	cam[0] = new GLMCamera(GLMCamera::FRUSTUM, scrWidth, scrHeight, -1.0f, 1.0f,
			-1.0f, 1.0f,                // left, right, bottom, top
			0.f, 0.f, 1.f,                           // camPos
			lookAt[0].x, lookAt[0].y, lookAt[0].z,   // lookAt
			upVec[0].x, upVec[0].y, upVec[0].z, 1.f, 100.f);

	leftScreenDim = glm::vec2(720.f, 480.f);
	centerScreenDim = glm::vec2(600.f, 480.f);
	rightScreenDim = glm::vec2(720.f, 480.f);

	modMatrices = new glm::mat4[nrCameras];
	viewMatrices = new glm::mat4[nrCameras];
	projMatrices = glm::mat4(1.f);

	for (short i = 0; i < nrCameras; i++)
	{
		modMatrices[i] = glm::mat4(1.f);
		viewMatrices[i] = glm::mat4(1.f);
	}

	// matrize für die linke seite, dreh alles um die y-Achse um 90grad im gegenuhrzeigersinn
	modMatrices[0] = glm::rotate(modMatrices[0], static_cast<float>(M_PI * 0.4),
			glm::vec3(0.f, 1.f, 0.f));

	// matrize für die rechte seite, dreh alles um die y-Achse um 90grad im gegenuhrzeigersinn
	modMatrices[2] = glm::rotate(modMatrices[2],
			static_cast<float>(M_PI * -0.4), glm::vec3(0.f, 1.f, 0.f));

	quad->setColor(0.f, 0.f, 0.f, 0.f);

	mappingShader = new Shaders("shaders/cam3scalerTex.vert",
			"shaders/cam3scalerTex.vert", "shaders/cam3scalerTex.vert", true);
	mappingShader->link();
}

Cs3InCubeScaler::~Cs3InCubeScaler()
{
	delete[] cam;
}

//--------------------------------------------------------------------------------

void Cs3InCubeScaler::initProto(ShaderProto* _proto)
{
//        lightProto[_scene->protoName]->defineVert(false);
//        if (!_proto->enableShdrType[GEOMETRY]) lightProto[_scene->protoName]->defineGeom(false);
//        lightProto[_scene->protoName]->defineFrag(false);
//        _proto->assemble();
}

//--------------------------------------------------------------------------------
/*
 void Cs3InCubeScaler::clearScreen()
 {
 glScissor(0, 0, scrWidth, scrHeight);

 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 
 quad->setColor(0.f, 0.f, 0.f, 1.f - osc->feedback),
 
 glViewportIndexedf(0, 0.f, 0.f, scrWidth, scrHeight);
 
 glClear(GL_DEPTH_BUFFER_BIT);   // necessary, sucks performance
 colShader->begin();
 fullScrCam->sendIdentMtr(colShader->getProgram(), "m_pvm");
 
 glDepthMask(GL_FALSE);
 glDisable(GL_DEPTH_TEST);
 quad->draw();
 glDepthMask(GL_TRUE);
 glEnable(GL_DEPTH_TEST);
 
 colShader->end();
 }
 */
//--------------------------------------------------------------------------------
void Cs3InCubeScaler::render(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	glScissor(0, 0, scrWidth, scrHeight);

	float yOffset = scrHeight - leftScreenDim.y - rightScreenDim.y;

	_scene->updateOscPar(time, dt);
	_scene->update(time, dt);

	// set viewports
	// starts from left down
	glViewportIndexedf(0, 0.f, rightScreenDim.y + yOffset, leftScreenDim.x, leftScreenDim.y);      // left top
	glViewportIndexedf(1, rightScreenDim.x, rightScreenDim.y + yOffset, centerScreenDim.x, centerScreenDim.y);  // right top
	glViewportIndexedf(2, 0.f, yOffset, rightScreenDim.x, rightScreenDim.y); // left bottom

	mappingShader->begin();
	mappingShader->setUniformMatrix4fv("model_matrix_g", &modMatrices[0][0][0], nrCameras);
	mappingShader->setUniformMatrix4fv("view_matrix_g", &viewMatrices[0][0][0], nrCameras);
	mappingShader->setUniformMatrix4fv("projection_matrix", &projMatrices[0][0]);

	mappingShader->setUniform1f("brightness", osc->totalBrightness);
	cp.camId = id;

	_scene->draw(time, dt, &cp, mappingShader);

}

//--------------------------------------------------------------------------------

void Cs3InCubeScaler::renderFbos(double time, double dt, unsigned int ctxNr)
{
}

//--------------------------------------------------------------------------------

void Cs3InCubeScaler::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs3InCubeScaler::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs3InCubeScaler::onKey(int key, int scancode, int action, int mods)
{
	// call onKey Function of active SceneNode
}

//--------------------------------------------------------------------------------

void Cs3InCubeScaler::mouseBut(GLFWwindow* window, int button, int action,
		int mods)
{
}

//--------------------------------------------------------------------------------

void Cs3InCubeScaler::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
}

//--------------------------------------------------------------------------------

void Cs3InCubeScaler::clearFbo()
{
}
}
