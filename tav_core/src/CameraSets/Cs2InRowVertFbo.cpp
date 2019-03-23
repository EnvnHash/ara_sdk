//
//  Cs2InRowVertFbo.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.

#define STRINGIFY(A) #A

#include "pch.h"
#include "Cs2InRowVertFbo.h"

namespace tav
{
Cs2InRowVertFbo::Cs2InRowVertFbo(sceneData* _scd, OSCData* _osc,
		GWindowManager* _winMan) :
		CameraSet(2, _scd, _osc), winMan(_winMan), renderMode(DRAW)
{
	id = _scd->id;

	cam = new GLMCamera*[nrCameras];
	upVec = new glm::vec3[nrCameras];
	camPos = new glm::vec3[nrCameras];

	float tops[2] = { 1.f, 0.f };
	float bottoms[2] = { 0.f, -1.f };

	lookAt[0] = glm::vec3(0.f, 0.f, 0.f);
	lookAt[1] = glm::vec3(0.f, 0.f, 0.f);

	upVec[0] = glm::vec3(1.f, 0.f, 0.f);
	upVec[1] = glm::vec3(1.f, 0.f, 0.f);

	//float aspect = float(scd->screenWidth / nrCameras) / float(scd->screenHeight);

	for (int i = 0; i < 2; i++)
		cam[i] = new GLMCamera(GLMCamera::FRUSTUM, scd->screenWidth,
				scd->screenHeight,
				//int(10000.f * widths[i]), int(tunnelHeight * 10000.f),
				-1.f, 1.f,   // left, right
				bottoms[i], tops[i],   // bottom, top
				0.f, 0.f, 1.f,	// camPos
				lookAt[i].x, lookAt[i].y, lookAt[i].z,	// lookAt
				upVec[i].x, upVec[i].y, upVec[i].z,	// upVecs
				1.f, 100.f);  	// upVec
	setupCamPar();

	// first render all 3 cameras (3 sides of the tunnel) into one fbo
	// then render each wall and back with two fbos
	// all screens exist as one xscreen (xinerama)
	fboDedist = new FboDedistPersp(_scd, _winMan, _scd->fboWidth, _scd->fboHeight);

	for (int i = 0; i < nrCameras; i++)
		fboDedist->add(_winMan->getWindows()->at(0), cam[i],
				float(scd->screenWidth / nrCameras), float(scd->screenHeight),
				float(i * scd->screenWidth / nrCameras), 0);

	setupFunc = std::bind(&CameraSet::setupCamPar, this, true);
	fboDedist->setCamSetupFunc(&setupFunc);
	fboDedist->setCalibFileName((*_scd->dataPath) + "/calib_cam/" + _scd->setupName + "_Cs2InRowVertFbo.yml");
}

//--------------------------------------------------------------------------------

Cs2InRowVertFbo::~Cs2InRowVertFbo()
{
}

//--------------------------------------------------------------------------------

void Cs2InRowVertFbo::initProto(ShaderProto* _proto)
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

void Cs2InRowVertFbo::clearFbo()
{
	fboDedist->bindFbo();
	fboDedist->clearFbo();
	fboDedist->unbindActFbo();
}

//--------------------------------------------------------------------------------

void Cs2InRowVertFbo::render(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	/*
	 cam = new GLMCamera*[nrCameras];
	 upVec = new glm::vec3[nrCameras];
	 camPos = new glm::vec3[nrCameras];

	 float tops[2] = { 1.f, 0.f };
	 float bottoms[2] = { 0.f, -1.f };

	 lookAt[0] = glm::vec3(0.f, 0.f, 0.f);
	 lookAt[1] = glm::vec3(0.f, 0.f, 0.f);

	 upVec[0] = glm::vec3(1.f, 0.f, 0.f);
	 upVec[1] = glm::vec3(1.f, 0.f, 0.f);

	 float aspect = float(scd->screenWidth / nrCameras) / float(scd->screenHeight);

	 for (int i=0;i<2;i++)
	 cam[i] =  new GLMCamera(GLMCamera::FRUSTUM,
	 scd->screenWidth, scd->screenHeight,
	 //int(10000.f * widths[i]), int(tunnelHeight * 10000.f),
	 -1.f, 1.f,   // left, right
	 bottoms[i], tops[i],   // bottom, top
	 0.f, 0.f, 1.f,	// camPos
	 lookAt[i].x, lookAt[i].y, lookAt[i].z,	// lookAt
	 upVec[i].x, upVec[i].y, upVec[i].z,	// upVecs
	 1.f, 100.f);  	// upVec
	 setupCamPar();
	 */

	// call update method of the scene, if blending is enabled
	// this already gets called in the scene blender. the resulting morphscene
	// will have a empty update method
	_scene->updateOscPar(time, dt);
	_scene->update(time, dt);

	// ------ render to FBO -------------

	if (lightProto.find(_scene->protoName) != lightProto.end())
	{
		fboDedist->bindFbo();

		lightProto[_scene->protoName]->shader->begin();
		lightProto[_scene->protoName]->sendPar(nrCameras, cp, osc, time);

		const float wot = float(scd->screenWidth / nrCameras);

		// set viewports
		for (short i = 0; i < nrCameras; i++)
			glViewportIndexedf(i, wot * float(i), 0.f, wot, scd->screenHeight); // 2 screens left

		// die groesse des FBO in den gezeichnet wird, sollte der groesse
		// des screens im setup xml entsprechen
		cp.actFboSize = fboDedist->getFboSize(0);
		cp.camId = id;

		lightProto[_scene->protoName]->sendPar(nrCameras, cp, osc, time); // 1 cameras
		_scene->draw(time, dt, &cp, lightProto[_scene->protoName]->shader);

		fboDedist->unbindActFbo();
	}
	else
	{
		printf("CameraSet Error: couldn´t get LightProto \n");
	}
}

//--------------------------------------------------------------------------------

void Cs2InRowVertFbo::renderFbos(double time, double dt, unsigned int ctxNr)
{
	// ------ render with perspective Dedistortion each part -------------
	/*
	 delete tunVisCam;

	 tunVisCam = new GLMCamera(
	 GLMCamera::PERSPECTIVE,
	 scd->screenWidth,
	 scd->screenHeight,
	 -1.f, 1.f,  		 // left, right
	 -1.f, 1.f,      	 // bottom, top
	 0.f, 0.f, 1.f,	// camPos
	 0.f, 0.f, 0.f,	// lookAt
	 0.f, 1.f, 0.f,
	 0.5f, 100.f, osc->zoom * 180.f);  	// upVec
	 */

	//glm::mat4 modMatr = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, osc->blurOffs - 1.f));
	//modMatr = glm::rotate(modMatr, float(M_PI) * osc->blurFdbk, glm::vec3(0.f, 1.f, 0.f));
	//tunVisCam->setModelMatr(modMatr);
	switch (renderMode)
	{
	case DRAW:
		fboDedist->drawAllFboViews();
		break;

	case TESTPIC:
		fboDedist->drawTestPic();
		break;
	}
}

//--------------------------------------------------------------------------------

void Cs2InRowVertFbo::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs2InRowVertFbo::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs2InRowVertFbo::onKey(int key, int scancode, int action, int mods)
{
	// call onKey Function of active SceneNode
	fboDedist->onKey(key, scancode, action, mods);
}

//--------------------------------------------------------------------------------

void Cs2InRowVertFbo::mouseBut(GLFWwindow* window, int button, int action,
		int mods)
{
	fboDedist->mouseBut(window, button, action, mods);
}

//--------------------------------------------------------------------------------

void Cs2InRowVertFbo::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
	fboDedist->mouseCursor(window, xpos, ypos);
}
}
