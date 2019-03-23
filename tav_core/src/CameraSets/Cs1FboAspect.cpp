/*
*	Cs1FboAspectAspectQuad.cpp
*	tav_gl4
*
*	Created by Sven Hahne on 14.08.14.
*	Copyright (c) 2014 Sven Hahne. All rights reserved.
*
*	Standard CameraSetup, die Kamera steht auf (0|0|1) und sieht auf den Ursprung
*	sichtbarer Bereich ist ca. von (-1|+1)|(-1|+1)|(0|-100)
*	je nach Seitenverhältnis. Near ist 1.f und Far 100.0f (sichtbarer Bereich beginnt bei z=0)
*
*	Rendert in einen FBO mit aspect relativem Frustum (korrekte proportionen)
*	Keine Aspect Korrektur
*
*/


#include "Cs1FboAspect.h"

namespace tav
{

Cs1FboAspect::Cs1FboAspect(sceneData* _scd, OSCData* _osc, std::vector<fboView*>* _fboViews,
		GWindowManager* _winMan) :
		CameraSet(1, _scd, _osc), winMan(_winMan)
{
	cam = new GLMCamera*[nrCameras];
	id = _scd->id;

	fboSize = glm::vec2(_scd->fboWidth, _scd->fboHeight);
	fboDedist = new FboDedistPersp(_scd, _winMan, _scd->fboWidth, _scd->fboHeight);
	fboDedist->setCamSetupFunc(&setupFunc);

	float aspect = fboSize.x / fboSize.y;
	lookAt[0] = glm::vec3(0.f, 0.f, 0.f);
	upVec[0] = glm::vec3(0.f, 1.f, 0.f);
	cam[0] = new GLMCamera(GLMCamera::FRUSTUM,
			scrWidth, scrHeight,
			-aspect, aspect, -1.0f, 1.0f,                // left, right, bottom, top
			0.f, 0.f, 1.f,                           // camPos
			lookAt[0].x, lookAt[0].y, lookAt[0].z,   // lookAt
			upVec[0].x, upVec[0].y, upVec[0].z, 1.f, 100.f);

	quad->setColor(0.f, 0.f, 0.f, 0.f);

	setupCamPar();
	setupFunc = std::bind(&CameraSet::setupCamPar, this, true);	// function to calculate the matrices of the CameraSet
}

//--------------------------------------------------------------------------------

void Cs1FboAspect::initProto(ShaderProto* _proto)
{
	_proto->defineVert(false);
	if (!_proto->enableShdrType[GEOMETRY]) _proto->defineGeom(false);
	_proto->defineFrag(false);
	_proto->assemble();
}

//--------------------------------------------------------------------------------

void Cs1FboAspect::render(SceneNode* _scene, double time, double dt, unsigned int ctxNr)
{
	// check if the fboViews changed and update if necessary
	for (std::vector<fboView*>::iterator it = osc->fboViews->begin(); it != osc->fboViews->end(); ++it)
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

		cp.actFboSize = fboSize;
		cp.camId = id;
		_scene->draw(time, dt, &cp, lightProto[_scene->protoName]->shader);

		fboDedist->unbindFbo();

	} else printf("CameraSet Error: couldn´t get LightProto \n");
}

//--------------------------------------------------------------------------------

void Cs1FboAspect::renderFbos(double time, double dt, unsigned int ctxNr)
{
	// ------ render with perspective Dedistortion -------------

	fboDedist->drawAllFboViews();
//        fboDedist->drawTestPic();

#ifdef HAVE_OPENCV
	if(reqSnapshot)
	{
		std::cout << "snapshot requested!!!" << std::endl;
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		 cv::Mat bigpic (fboSize.x, fboSize.y, CV_8UC4);

		 glBindTexture(GL_TEXTURE_2D, fboDedist->getFboTex(0));
		 glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, bigpic.data);

		 char fileName [100];
		 sprintf( fileName, "/home/sven/tav_data/screenshots/out%d%d%f.jpg",
			 static_cast<int>( ( static_cast<double>(savedNr) / 10.0 ) ),
			 savedNr % 10,
			 time
			 );

		 cv::imwrite(fileName, bigpic);

		 printf("image written: %s\n", fileName);
		 reqSnapshot = false;
	}
#endif
}

//--------------------------------------------------------------------------------

void Cs1FboAspect::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs1FboAspect::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs1FboAspect::onKey(int key, int scancode, int action, int mods)
{
	// call onKey Function of active SceneNode
	fboDedist->onKey(key, scancode, action, mods);

	if (action == GLFW_PRESS && mods == GLFW_MOD_SHIFT)
	{
		switch (key)
		{
		case GLFW_KEY_D:
			std::cout << "req snapshot" << std::endl;
			reqSnapshot = true;
			break;
		}
	}
}

//--------------------------------------------------------------------------------

void Cs1FboAspect::mouseBut(GLFWwindow* window, int button, int action, int mods)
{
	fboDedist->mouseBut(window, button, action, mods);
}

//--------------------------------------------------------------------------------

void Cs1FboAspect::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
	fboDedist->mouseCursor(window, xpos, ypos);
}

//--------------------------------------------------------------------------------

Cs1FboAspect::~Cs1FboAspect()
{
	delete[] cam;
}

//--------------------------------------------------------------------------------

void Cs1FboAspect::clearFbo()
{
	fboDedist->clearFbo();
}
}
