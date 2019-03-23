//
//  Cs3FloorWall.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  2 projektionen auf den boden eine an die Wand
//  projektionen boden: Kopf an Kopf, 2 x Hochformat in Blickrichtung Wand

#define STRINGIFY(A) #A

#include "pch.h"
#include "Cs3FloorWall.h"

namespace tav
{
Cs3FloorWall::Cs3FloorWall(sceneData* _scd, OSCData* _osc,
		GWindowManager* _winMan) :
		CameraSet(3, _scd, _osc), winMan(_winMan)
{
	id = _scd->id;

	cam = new GLMCamera*[nrCameras];
	upVec = new glm::vec3[nrCameras];
	camPos = new glm::vec3[nrCameras];

	float top[3] =
	{ 0.f, 0.f, 1.f };
	float bottom[3] =
	{ -1.f, -1.f, -1.f };

	lookAt[0] = glm::vec3(0.f, 0.f, 0.f);
	lookAt[1] = glm::vec3(0.f, 0.f, 0.f);
	lookAt[2] = glm::vec3(0.f, 0.f, 0.f);

	upVec[0] = glm::vec3(1.f, 0.f, 0.f);
	upVec[1] = glm::vec3(-1.f, 0.f, 0.f);
	upVec[2] = glm::vec3(0.f, 1.f, 0.f);

	camPos[0] = glm::vec3(0.f, 2.f, 0.f);
	camPos[1] = glm::vec3(0.f, 2.f, 0.f);
	camPos[2] = glm::vec3(0.f, 0.f, 2.f);

	//float fovY = 60.f;
	//float aspect = float(_scd->screenWidth / 3) / float(_scd->screenHeight);

	// einheits wuerfel soll zu sehen sein.
	// deshalb camera pos z = 2.f;
	// left und right statt 1 deshalb tan(alpha)
	float alpha = std::atan2(1.f, 2.f);
	float uniScale = std::tan(alpha);

	for (int i=0; i<nrCameras; i++)
	{
		cam[i] = new GLMCamera(GLMCamera::FRUSTUM,
				_scd->screenWidth / nrCameras, _scd->screenHeight,
				-1.f * uniScale, 1.f * uniScale,            // left, right
				bottom[i] * uniScale, top[i] * uniScale,    // bottom, top
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
	for (int i=0; i<nrCameras; i++)
	{
		fboDedist->add(_winMan->getWindows()->at(0), cam[i],
				float(scd->screenWidth / nrCameras), float(scd->screenHeight),
				float(i) * float(scd->screenWidth / nrCameras), 0);
	}

	fboDedist->setCalibFileName(
			(*scd->dataPath) + "/calib_cam/" + scd->setupName
					+ "_Cs3FloorWall.yml");
	setupFunc = std::bind(&CameraSet::setupCamPar, this, true);
	fboDedist->setCamSetupFunc(&setupFunc);

	gradTex = new TextureManager();
	gradTex->loadTexture2D(*scd->dataPath + "/textures/grad_hor.png");
	gradTex->setWraping(GL_CLAMP_TO_BORDER);

	texShdr = shCol->getStdTex();
	gradquad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f,
			0.f, 0.f, 0.f);
}

//--------------------------------------------------------------------------------

Cs3FloorWall::~Cs3FloorWall()
{
}

//--------------------------------------------------------------------------------

void Cs3FloorWall::initProto(ShaderProto* _proto)
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

void Cs3FloorWall::render(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	// call update method of the scene, if blending is enabled
	// this already gets called in the scene blender. the resulting morphscene
	// will have a empty update method
	_scene->updateOscPar(time, dt);
	_scene->update(time, dt);

	if (lightProto.find(_scene->protoName) != lightProto.end())
	{
		// ------ render to FBO -------------

		lightProto[_scene->protoName]->shader->begin();
		lightProto[_scene->protoName]->sendPar(nrCameras, cp, osc, time);

		const float wot = float(scd->screenWidth / nrCameras);

		// set viewport floor 2 screens in one fbo
		for (short i = 0; i < nrCameras; i++)
			glViewportIndexedf(i, wot * float(i), 0.f, wot, scd->screenHeight);

		lightProto[_scene->protoName]->sendPar(nrCameras, cp, osc, time); // 1 cameras
		cp.camId = id;

		_scene->draw(time, dt, &cp, lightProto[_scene->protoName]->shader);

		// draw gradient
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glViewportIndexedf(0, 0.f, 0.f, wot, scd->screenHeight);

		// draw gradient masks on the top
		texShdr->begin();
		texShdr->setUniformMatrix4fv("m_pvm", fboDedist->getGradQuadMatPtr(0, FboDedistPersp::TOP));
		gradTex->bind(0);
		gradquad->draw();

		glViewportIndexedf(0, wot, 0.f, wot, scd->screenHeight);
		texShdr->setUniformMatrix4fv("m_pvm", fboDedist->getGradQuadMatPtr(1, FboDedistPersp::TOP));
		gradquad->draw();

		for (short i = 0; i < nrCameras; i++)
			fboDedist->drawCorners(i);
	}
	else
	{
		printf("CameraSet Error: couldn´t get LightProto \n");
	}
}

//--------------------------------------------------------------------------------

void Cs3FloorWall::renderFbos(double time, double dt, unsigned int ctxNr)
{
	// ------ render with perspective Dedistortion each part -------------

	//  fboDedist->drawAllFbo();
}

//--------------------------------------------------------------------------------

void Cs3FloorWall::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs3FloorWall::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs3FloorWall::onKey(int key, int scancode, int action, int mods)
{
	// call onKey Function of active SceneNode
	fboDedist->onKey(key, scancode, action, mods);
}

//--------------------------------------------------------------------------------

void Cs3FloorWall::mouseBut(GLFWwindow* window, int button, int action,
		int mods)
{
	fboDedist->mouseBut(window, button, action, mods);
}

//--------------------------------------------------------------------------------

void Cs3FloorWall::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
	fboDedist->mouseCursor(window, xpos, ypos);
}

//--------------------------------------------------------------------------------

void Cs3FloorWall::clearFbo()
{
}
}
