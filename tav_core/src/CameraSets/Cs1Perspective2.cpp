//
//  Cs1Perspective2.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Standard CameraSetup, die Kamera befindet sich ein wenig
//  "ausserhalb des Bilderschirms", und sieht auf den Ursprung
//  sichtbarer Bereich ist ca. von (-1|+1)|(-1|+1)|(0|-100)
//  je nach Seitenverhältnis. Near ist 0.1f und Far 100.0f
//

#include "pch.h"
#include "Cs1Perspective2.h"

namespace tav
{
Cs1Perspective2::Cs1Perspective2(sceneData* _scd, OSCData* _osc) :
		CameraSet(1, _scd, _osc)
{
	id = _scd->id;

	cam = new GLMCamera*[nrCameras];

	lookAt[0] = glm::vec3(0.f, 0.f, 0.f);
	upVec[0] = glm::vec3(0.f, 1.f, 0.f);

	float aspect = float(scd->screenWidth) / float(scd->screenHeight);

	cam[0] = new GLMCamera(GLMCamera::FRUSTUM, scrWidth, scrHeight, -aspect,
			aspect, -1.0f, 1.0f,                // left, right, bottom, top
			0.f, 0.f, 1.f,                           // camPos
			lookAt[0].x, lookAt[0].y, lookAt[0].z,   // lookAt
			upVec[0].x, upVec[0].y, upVec[0].z, 1.f, 100.f);

	quad->setColor(0.f, 0.f, 0.f, 0.f);
	setupCamPar();
}

Cs1Perspective2::~Cs1Perspective2()
{
	delete[] cam;
}

//--------------------------------------------------------------------------------

void Cs1Perspective2::initProto(ShaderProto* _proto)
{
	_proto->defineVert(false);
	if (!_proto->enableShdrType[GEOMETRY])
		_proto->defineGeom(false);
	_proto->defineFrag(false);
	_proto->assemble();
}

//--------------------------------------------------------------------------------

void Cs1Perspective2::render(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	glScissor(0, 0, scrWidth, scrHeight);

	// call update method of the scene, if blending is enabled
	// this already gets called in the scene blender. the resulting morphscene
	// will have a empty update method
	_scene->updateOscPar(time, dt);
	_scene->update(time, dt);

	if (lightProto.find(_scene->protoName) != lightProto.end())
	{
		lightProto[_scene->protoName]->preRender(_scene, time, dt); // pre rendering e.g. for shadow maps
		lightProto[_scene->protoName]->shader->begin();
		lightProto[_scene->protoName]->sendPar(1, cp, osc, time);   // 1 cameras

		cp.camId = id;
		cp.actFboSize = glm::vec2(float(scrWidth), float(scrHeight));

		_scene->draw(time, dt, &cp, lightProto[_scene->protoName]->shader);
	}
	else
	{
		printf("CameraSet Error: couldn´t get LightProto \n");
	}
}

//--------------------------------------------------------------------------------

void Cs1Perspective2::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs1Perspective2::renderFbos(double time, double dt, unsigned int ctxNr)
{
}

//--------------------------------------------------------------------------------

void Cs1Perspective2::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs1Perspective2::onKey(int key, int scancode, int action, int mods)
{
	// call onKey Function of active SceneNode
}

//--------------------------------------------------------------------------------

void Cs1Perspective2::mouseBut(GLFWwindow* window, int button, int action,
		int mods)
{
}

//--------------------------------------------------------------------------------

void Cs1Perspective2::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
}

//--------------------------------------------------------------------------------

void Cs1Perspective2::clearFbo()
{
}
}
