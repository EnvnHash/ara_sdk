//
//  Cs2InRowVert.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  2 projektionen auf den boden eine an die Wand
//  projektionen boden: Kopf an Kopf, 2 x Hochformat in Blickrichtung Wand

#define STRINGIFY(A) #A

#include "pch.h"
#include "Cs2InRowVert.h"

namespace tav
{
Cs2InRowVert::Cs2InRowVert(sceneData* _scd, OSCData* _osc, GWindowManager* _winMan) :
		CameraSet(2, _scd, _osc), winMan(_winMan)
{
	id = _scd->id;

	cam = new GLMCamera*[nrCameras];
	upVec = new glm::vec3[nrCameras];
	camPos = new glm::vec3[nrCameras];

	lookAt[0] = glm::vec3(0.f, 0.f, 0.f);
	lookAt[1] = glm::vec3(0.f, 0.f, 0.f);

	upVec[0] = glm::vec3(0.f, -1.f, 0.f);
	upVec[1] = glm::vec3(0.f, -1.f, 0.f);

	camPos[0] = glm::vec3(0.f, 0.f, 1.f);
	camPos[1] = glm::vec3(0.f, 0.f, 1.f);

	fovY = 27.26f;
	fovY = fovY / 360.f * M_PI * 2.f;

	float aspect = float(scd->screenWidth) / float(scd->screenHeight);
	float near = 1.f; // fix
	float top = std::tan(fovY) * near;
	float left = top * aspect;

	for (int i = 0; i < nrCameras; i++)
	{
		cam[i] = new GLMCamera(GLMCamera::FRUSTUM,
				_scd->screenWidth / nrCameras, _scd->screenHeight, -left, left, // left, right
				-top, top,   				// bottom, top
				camPos[i].x, camPos[i].y, camPos[i].z,      // camPos
				lookAt[i].x, lookAt[i].y, lookAt[i].z,      // lookAt
				upVec[i].x, upVec[i].y, upVec[i].z,         // upVec
				1.f, 100.f);
	}

	setupCamPar();

	// first render all 4 cameras (4 sides of the tunnel) into one fbo
	// then render each wall and roof with two fbos
	// the floor will be rendered as one screen
	// all screens exist as one xscreen (xinerama)
	fboDedist = new FboDedistPersp(_scd, _winMan, _scd->fboWidth, _scd->fboHeight);

	// floor 2 screens, wall 1 screen
	for (int i = 0; i < nrCameras; i++)
	{
		fboDedist->add(_winMan->getWindows()->at(0), cam[i],
				float(scd->screenWidth / nrCameras), float(scd->screenHeight),
				float(i) * float(scd->screenWidth / nrCameras), 0);
	}

	fboDedist->setCalibFileName((*scd->dataPath) + "/calib_cam/" + scd->setupName + "_Cs2InRowVert.yml");
	setupFunc = std::bind(&CameraSet::setupCamPar, this, true);
	fboDedist->setCamSetupFunc(&setupFunc);

	/*
	 texShdr = shCol->getStdTex();
	 floorTex = new TextureManager();
	 floorTex->loadTexture2D(*scd->dataPath+"/textures/tv.jpg");
	 testquad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
	 glm::vec3(0.f, 0.f, 1.f),
	 0.f, 0.f, 0.f, 1.f);
	 */
}

//--------------------------------------------------------------------------------

Cs2InRowVert::~Cs2InRowVert()
{
}

//--------------------------------------------------------------------------------

void Cs2InRowVert::initProto(ShaderProto* _proto)
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

void Cs2InRowVert::render(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	// call update method of the scene, if blending is enabled
	// this already gets called in the scene blender. the resulting morphscene
	// will have a empty update method
	_scene->updateOscPar(time, dt);
	_scene->update(time, dt);

	cam = new GLMCamera*[nrCameras];
	upVec = new glm::vec3[nrCameras];
	camPos = new glm::vec3[nrCameras];

	lookAt[0] = glm::vec3(0.f, 0.f, 0.f);
	lookAt[1] = glm::vec3(0.f, 0.f, 0.f);

	upVec[0] = glm::vec3(-1.f, 0.f, 0.f);
	upVec[1] = glm::vec3(-1.f, 0.f, 0.f);

	float aspect = float(scd->screenWidth / nrCameras)
			/ float(scd->screenHeight);
	float near = 1.f; // fix
	float top = std::tan(fovY) * near;
	float left = top * aspect;

	camPos[0] = glm::vec3(0.f, 0.f, 1.f);
	camPos[1] = glm::vec3(0.f, 0.f, 1.f);

	float offSetX[2] = { (osc->alpha - 0.5f) * 2.f, (osc->feedback - 0.5f) * 2.f };
	float offSetY[2] = { (osc->blurFboAlpha - 0.5f) * 2.f, (osc->blurFdbk - 0.5f) * 2.f };
	float scale[2] = { osc->totalBrightness, osc->rotYAxis * 2.f };

	for (int i = 0; i < nrCameras; i++)
	{
		cam[i] = new GLMCamera(GLMCamera::FRUSTUM, scd->screenWidth,
				scd->screenHeight, (-left + offSetX[i]) * scale[i],
				(left + offSetX[i]) * scale[i],            // left, right
				(-top + offSetY[i]) * scale[i], (top + offSetY[i]) * scale[i], // bottom, top
				camPos[i].x, camPos[i].y, camPos[i].z,      // camPos
				lookAt[i].x, lookAt[i].y, lookAt[i].z,      // lookAt
				upVec[i].x, upVec[i].y, upVec[i].z,         // upVec
				near, 100.f);
	}

	setupCamPar();

	// ------ render to FBO -------------

	if (lightProto.find(_scene->protoName) != lightProto.end())
	{
		fboDedist->bindFbo();
		fboDedist->clearFbo();

		lightProto[_scene->protoName]->shader->begin();
		lightProto[_scene->protoName]->sendPar(nrCameras, cp, osc, time);

		const float wot = float(scd->screenWidth / nrCameras);

		// set viewport floor 2 screens in one fbo
		for (short i = 0; i < nrCameras; i++)
			glViewportIndexedf(i, wot * float(i), 0.f, wot, scd->screenHeight);

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

void Cs2InRowVert::renderFbos(double time, double dt, unsigned int ctxNr)
{
	// ------ render with perspective Dedistortion each part -------------

	fboDedist->drawAllFboViews();
}

//--------------------------------------------------------------------------------

void Cs2InRowVert::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs2InRowVert::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs2InRowVert::onKey(int key, int scancode, int action, int mods)
{
	// call onKey Function of active SceneNode
	fboDedist->onKey(key, scancode, action, mods);
}

//--------------------------------------------------------------------------------

void Cs2InRowVert::mouseBut(GLFWwindow* window, int button, int action,
		int mods)
{
	fboDedist->mouseBut(window, button, action, mods);
}

//--------------------------------------------------------------------------------

void Cs2InRowVert::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
	fboDedist->mouseCursor(window, xpos, ypos);
}

//--------------------------------------------------------------------------------

void Cs2InRowVert::clearFbo()
{
}
}
