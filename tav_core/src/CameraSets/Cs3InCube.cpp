//
//  Cs3InCube.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  CameraSetup, für drei Bildschirme, die sich auf gleicher Höhe
//  nebeneinander befinden, bzw. drei Beamer mit Overlap
//  sichtbarer Bereich ist ca. von (-1|+1)|(-1|+1)|(0|-100)
//  je nach Seitenverhältnis. Near ist 0.1f und Far 100.0f
//  scrWidth und scrHeight sollten sich auf die Groesse beider Bildschirme
//  zusammen beziehen
//

#include "pch.h"
#include "Cs3InCube.h"

namespace tav
{
Cs3InCube::Cs3InCube(sceneData* _scd, OSCData* _osc) :
		CameraSet(3, _scd, _osc)
{
	id = _scd->id;

	overlap = 0.1f;

	cam = new GLMCamera*[nrCameras];

	lookAt[0] = glm::vec3(-1.f, 0.f, 0.f); // links
	lookAt[1] = glm::vec3(0.f, 0.f, -1.f); // vorne
	lookAt[2] = glm::vec3(1.f, 0.f, 0.f); // rechts

	for (int i = 0; i < nrCameras; i++)
		cam[i] = new GLMCamera(GLMCamera::FRUSTUM, scrWidth / nrCameras,
				scrHeight, -1.f, 1.f,                             // left, right
				-1.f, 1.f,                               // bottom, top
				0.f, 0.f, 0.f,                           // camPos
				lookAt[i].x, lookAt[i].y, lookAt[i].z);  // lookAt

	quad->setColor(0.f, 0.f, 0.f, 1.f);
	setupCamPar();
}

Cs3InCube::~Cs3InCube()
{
}

//--------------------------------------------------------------------------------

void Cs3InCube::initProto(ShaderProto* _proto)
{
	_proto->defineVert(true);
	if (!_proto->enableShdrType[GEOMETRY])
		_proto->defineGeom(true);
	_proto->defineFrag(true);

	_proto->asmblMultiCamGeo(); // add geo shader for multicam rendering
	_proto->enableShdrType[GEOMETRY] = true;

	_proto->assemble();
}

//--------------------------------------------------------------------------------
/*
 void Cs3InCube::clearScreen()
 {
 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

 quad->setColor(0.f, 0.f, 0.f, 1.f - osc->feedback),

 glViewportIndexedf(0,       0.f, 0.f, scrWidth, scrHeight);

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
void Cs3InCube::render(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	// call update method of the scene, if blending is enabled
	// this already gets called in the scene blender. the resulting morphscene
	// will have a empty update method
	_scene->updateOscPar(time, dt);
	_scene->update(time, dt);

	if (lightProto.find(_scene->protoName) != lightProto.end())
	{
		lightProto[_scene->protoName]->preRender(_scene, time, dt);
		lightProto[_scene->protoName]->shader->begin();
		lightProto[_scene->protoName]->sendPar(nrCameras, cp, osc, time);

		// set viewports
		const float wot = float(scrWidth) / 3.f;
		glViewportIndexedf(0, 0.f, 0.f, wot, scrHeight);
		glViewportIndexedf(1, wot, 0.f, wot, scrHeight);
		glViewportIndexedf(2, wot * 2.f, 0.f, wot, scrHeight);
		cp.camId = id;

		_scene->draw(time, dt, &cp, lightProto[_scene->protoName]->shader);
	}
	else
	{
		printf("CameraSet Error: couldn´t get LightProto \n");
	}
}

//--------------------------------------------------------------------------------

void Cs3InCube::renderFbos(double time, double dt, unsigned int ctxNr)
{
}

//--------------------------------------------------------------------------------

void Cs3InCube::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs3InCube::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs3InCube::onKey(int key, int scancode, int action, int mods)
{
}

//--------------------------------------------------------------------------------

void Cs3InCube::mouseBut(GLFWwindow* window, int button, int action, int mods)
{
}

//--------------------------------------------------------------------------------

void Cs3InCube::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
}

//--------------------------------------------------------------------------------

void Cs3InCube::clearFbo()
{
}

}
