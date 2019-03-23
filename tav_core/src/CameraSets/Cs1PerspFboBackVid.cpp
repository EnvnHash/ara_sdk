//
//  Cs1PerspFboBackVid.cpp
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
#include "Cs1PerspFboBackVid.h"

namespace tav
{
Cs1PerspFboBackVid::Cs1PerspFboBackVid(sceneData* _scd, OSCData* _osc, std::vector<fboView*>* _fboViews,
		GWindowManager* _winMan) :
		CameraSet(1, _scd, _osc), winMan(_winMan)
{
	id = _scd->id;
	cam = new GLMCamera*[nrCameras];
	float aspect = float(scd->screenWidth) / float(scd->screenHeight);

	lookAt[0] = glm::vec3(0.f, 0.f, 0.f);
	upVec[0] = glm::vec3(0.f, 1.f, 0.f);
	cam[0] = new GLMCamera(GLMCamera::FRUSTUM, scrWidth, scrHeight, -aspect,
			aspect, -1.0f, 1.0f,                // left, right, bottom, top
			0.f, 0.f, 1.f,                           // camPos
			lookAt[0].x, lookAt[0].y, lookAt[0].z,   // lookAt
			upVec[0].x, upVec[0].y, upVec[0].z, 1.f, 100.f);

	quad->setColor(0.f, 0.f, 0.f, 0.f);

	flipQuad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f,
			0.f, 0.f, 1.f, nullptr, 1, true);

	setupCamPar();
	setupFunc = std::bind(&CameraSet::setupCamPar, this, true);

	fboDedist = new FboDedistPersp(_scd, _winMan, _scd->fboWidth, _scd->fboHeight);
	fboDedist->setCamSetupFunc(&setupFunc);

	// background video specific stuff
	stdTex = _scd->shaderCollector->getStdTex();

	backVidPath = (*scd->backVidPath).c_str();
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBackVid::initProto(ShaderProto* _proto)
{
	_proto->defineVert(false);
	if (!_proto->enableShdrType[GEOMETRY])
		_proto->defineGeom(false);
	_proto->defineFrag(false);
	_proto->assemble();
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBackVid::render(SceneNode* _scene, double time, double dt,
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
		fboDedist->clearFbo();

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

void Cs1PerspFboBackVid::updateBackVid(double time)
{
	if (!cap)
	{
		std::cout << "Init Background Video" << std::endl;

		cap = new cv::VideoCapture(backVidPath); // open the default camera
		if (!cap->isOpened())  // check if we succeeded
			std::cout << "couldnt open video" << std::endl;

		uploadTexture = new TextureManager();
		uploadTexture->allocate((int) cap->get(cv::CAP_PROP_FRAME_WIDTH),
				(int) cap->get(cv::CAP_PROP_FRAME_HEIGHT),
				GL_RGBA8, GL_BGRA, GL_TEXTURE_2D, GL_UNSIGNED_BYTE);

		std::cout << "capture and texture inited" << std::endl;
		std::cout << "act frame pos " << cap->get(cv::CAP_PROP_POS_MSEC)
				<< std::endl;
	}

	//std::cout << "processing " << processing << std::endl;

	// update background video
	if (!processing
			&& cap->get(cv::CAP_PROP_POS_MSEC) * 0.001 < (time - zeroTime))
	{
		if (cap->get(cv::CAP_PROP_POS_FRAMES)
				>= cap->get(cv::CAP_PROP_FRAME_COUNT))
		{
			cap->set(cv::CAP_PROP_POS_FRAMES, 0);
			zeroTime = time;
		}
		//std::cout << "start capture thread " << std::endl;
		std::thread m_Thread = std::thread(
				&Cs1PerspFboBackVid::processQueue, this);
	}

	if (hasFrame && new_frame != old_frame)
	{
		mutex.lock();
		uploadTexture->bind(0);
		glTexSubImage2D(
				GL_TEXTURE_2D,             // target
				0,                          // First mipmap level
				0,
				0,                       // x and y offset
				(int) cap->get(cv::CAP_PROP_FRAME_WIDTH),
				(int) cap->get(cv::CAP_PROP_FRAME_HEIGHT),
				GL_BGR,
				GL_UNSIGNED_BYTE, &frame.data[0]);

		mutex.unlock();
		hasTex = true;

		old_frame = new_frame;
	}
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBackVid::renderFbos(double time, double dt, unsigned int ctxNr)
{
	// ------ render with perspective Dedistortion -------------

	fboDedist->drawAllFboViews();
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBackVid::preDisp(double time, double dt)
{
	updateBackVid(time);

	// draw background
	if (hasTex)
	{
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);

		stdTex->begin();
		stdTex->setIdentMatrix4fv("m_pvm");
		stdTex->setUniform1i("tex", 0);
		uploadTexture->bind(0);
		flipQuad->draw();
	}
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBackVid::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBackVid::processQueue()
{
	processing = true;
	mutex.lock();
	(*cap) >> frame; // get the first frame
	new_frame++;
	if (!hasFrame)
		hasFrame = true;
	mutex.unlock();
	processing = false;
}

//---------------------------------------------------------

void Cs1PerspFboBackVid::onKey(int key, int scancode, int action, int mods)
{
	// call onKey Function of active SceneNode
	fboDedist->onKey(key, scancode, action, mods);
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBackVid::mouseBut(GLFWwindow* window, int button, int action,
		int mods)
{
	fboDedist->mouseBut(window, button, action, mods);
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBackVid::mouseCursor(GLFWwindow* window, double xpos,
		double ypos)
{
	fboDedist->mouseCursor(window, xpos, ypos);
}

//--------------------------------------------------------------------------------

Cs1PerspFboBackVid::~Cs1PerspFboBackVid()
{
	delete[] cam;
}

//--------------------------------------------------------------------------------

void Cs1PerspFboBackVid::clearFbo()
{
	fboDedist->clearFbo();
}
}
