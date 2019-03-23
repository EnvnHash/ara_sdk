//
//  Cs1PerspFboBack.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 22.12.15.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Standard CameraSetup, die Kamera befindet sich ein wenig
//  "ausserhalb des Bilderschirms", und sieht auf den Ursprung
//  sichtbarer Bereich ist ca. von (-1|+1)|(-1|+1)|(0|-100)
//  je nach Seitenverhältnis. Near ist 0.1f und Far 100.0f
//

#include "pch.h"
#include "Cs1PerspFboBack.h"

namespace tav
{
Cs1PerspFboBack::Cs1PerspFboBack(sceneData* _scd, OSCData* _osc, std::vector<fboView*>* _fboViews,
		GWindowManager* _winMan) :
		CameraSet(1, _scd, _osc), winMan(_winMan)
{
	id = _scd->id;
	cam = new GLMCamera*[nrCameras];

	lookAt[0] = glm::vec3(0.f, 0.f, 0.f);
	upVec[0] = glm::vec3(0.f, 1.f, 0.f);
	cam[0] = new GLMCamera(GLMCamera::FRUSTUM, scrWidth, scrHeight, -1.f, 1.0f,
			-1.0f, 1.0f,                // left, right, bottom, top
			0.f, 0.f, 1.f,                           // camPos
			lookAt[0].x, lookAt[0].y, lookAt[0].z,   // lookAt
			upVec[0].x, upVec[0].y, upVec[0].z, 1.f, 100.f);

	quad->setColor(0.f, 0.f, 0.f, 0.f);

	setupCamPar();
	setupFunc = std::bind(&CameraSet::setupCamPar, this, true);

	fboDedist = new FboDedistPersp(_scd, _winMan, _scd->fboWidth, _scd->fboHeight);
	fboDedist->setCamSetupFunc(&setupFunc);

	stdTex = _scd->shaderCollector->getStdTex();
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBack::initProto(ShaderProto* _proto)
{
	_proto->defineVert(false);
	if (!_proto->enableShdrType[GEOMETRY])
		_proto->defineGeom(false);
	_proto->defineFrag(false);
	_proto->assemble();
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBack::render(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	// check if the fboViews changed and update if necessary
	for (std::vector<fboView*>::iterator it = osc->fboViews->begin();
			it != osc->fboViews->end(); ++it)
	{
		if ((*it)->update && (*it)->srcCamId == id)
		{
			fboDedist->updtFboView(it);
			(*it)->update = false;
		}
	}

	//  glScissor(0,0, scrWidth, scrHeight);

	// call update method of the scene, if blending is enabled
	// this already gets called in the scene blender. the resulting morphscene
	// will have a empty update method
	_scene->updateOscPar(time, dt);
	_scene->update(time, dt);

	if (lightProto.find(_scene->protoName) != lightProto.end())
	{
		lightProto[_scene->protoName]->preRender(_scene, time, dt); // pre rendering e.g. for shadow maps

		// ------ render to FBO -------------

		fboDedist->bindFbo();

		lightProto[_scene->protoName]->shader->begin();
		lightProto[_scene->protoName]->sendPar(1, cp, osc, time);   // 1 cameras

		cp.camId = id;
		cp.actFboSize = glm::vec2(float(scrWidth), float(scrHeight));

		_scene->draw(time, dt, &cp, lightProto[_scene->protoName]->shader);

		fboDedist->unbindFbo();
	}
	else
	{
		printf("CameraSet Error: couldn´t get LightProto \n");
	}
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBack::renderFbos(double time, double dt, unsigned int ctxNr)
{
	// draw background
	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);

	scd->backTex->bind(0);
	quad->draw();

	// ------ render with perspective Dedistortion -------------

	fboDedist->drawAllFboViews();
//        fboDedist->drawTestPic();

}

//--------------------------------------------------------------------------------

void Cs1PerspFboBack::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBack::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBack::onKey(int key, int scancode, int action, int mods)
{
	// call onKey Function of active SceneNode
	fboDedist->onKey(key, scancode, action, mods);
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBack::mouseBut(GLFWwindow* window, int button, int action,
		int mods)
{
	fboDedist->mouseBut(window, button, action, mods);
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBack::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
	fboDedist->mouseCursor(window, xpos, ypos);
}

//--------------------------------------------------------------------------------

Cs1PerspFboBack::~Cs1PerspFboBack()
{
	delete[] cam;
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBack::clearFbo()
{
	fboDedist->clearFbo();
}
}
