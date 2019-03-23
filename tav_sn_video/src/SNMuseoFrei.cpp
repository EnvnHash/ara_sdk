//
// SNMuseoFrei.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNMuseoFrei.h"

using namespace std;

namespace tav
{
SNMuseoFrei::SNMuseoFrei(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs), requestTrigVideo(true), blendTime(1.0),
		requestLoopVideo(false), debug(true), debugSwitchTime(8.0), lastDebugAct(0.0)
{

	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	stdTex = shCol->getStdTex();
	quad = static_cast<Quad*>(_scd->stdHFlipQuad);
	winMan = static_cast<GWindowManager*>(scd->winMan);
	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
		return this->onKey(key, scancode, action, mods); });

	loop.OpenFile(shCol, (*_scd->dataPath)+"/movies/test.mp4", 4, 640, 360, true, true);
	loop.start();

	trig_video = new FFMpegDecode();
	trig_video->setEndCbFunc(std::bind(&SNMuseoFrei::trig_video_endCb, this));
	trig_video->OpenFile(shCol, (*_scd->dataPath)+"/movies/00_Entrada_Loop_cut.m4v", 4, 640, 360, true, true);
	trig_video->start();

	blendVal = new AnimVal<float>(RAMP_LIN_UP, [this](){});
	blendVal->setInitVal(0.f);

	isBlending = false;
}

//------------------------------------------

void SNMuseoFrei::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	loop.shaderBegin(); // draw with conversion yuv -> rgb on gpu
	if (loop.getShader() != NULL)
		loop.getShader()->setUniform1f("alpha", 1.f - blendVal->getVal() );
	quad->draw(_tfo);


	trig_video->shaderBegin(); // draw with conversion yuv -> rgb on gpu
	if (trig_video->getShader() != NULL)
		trig_video->getShader()->setUniform1f("alpha", blendVal->getVal() );
	quad->draw(_tfo);

	//_shader->begin();
}

//------------------------------------------

void SNMuseoFrei::update(double time, double dt)
{
	loop.loadFrameToTexture(time);
	trig_video->loadFrameToTexture(time);

	if (debug && (time - lastDebugAct) > debugSwitchTime)
	{
		cout << "debug switch" << endl;
		requestTrigVideo = true;
		lastDebugAct = time;
	}

	if (requestTrigVideo)
	{
		requestTrigVideo = false;

		blendVal->setEndFunc([this](){ isBlending = false; });
		blendVal->setInitVal(0.f);
		blendVal->start(0.f, 1.f, blendTime, time, false);
		isBlending = true;

	//	trig_video.seek_frame(0, time);
		trig_video->stop();
		delete trig_video;

		trig_video = new FFMpegDecode();
		trig_video->setEndCbFunc(std::bind(&SNMuseoFrei::trig_video_endCb, this));
		trig_video->OpenFile(shCol, (*scd->dataPath)+"/movies/00_Entrada_Loop_cut.m4v", 4, 640, 360, true, true);
		trig_video->start();
	}

	if (requestLoopVideo)
	{
		requestLoopVideo = false;

		blendVal->setEndFunc([this](){ isBlending = false; });
		blendVal->setInitVal(1.f);
		blendVal->start(1.f, 0.f, blendTime, time, false);
		isBlending = true;
	}

	blendVal->update(time);
}

//----------------------------------------------------

void SNMuseoFrei::trig_video_endCb()
{
	cout << "SNMuseoFrei::trig_video_endCb " << endl;
	requestLoopVideo = true;

}

//----------------------------------------------------

void SNMuseoFrei::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
			case GLFW_KEY_UP :
				if (!isBlending){
					requestTrigVideo = true;
					printf("request change video\n");
				}
				break;

			case GLFW_KEY_DOWN :
				if (!isBlending){
					requestLoopVideo = true;
					printf("request change o looopp\n");
				}
				break;
		}
	}

	/*
	if (action == GLFW_RELEASE)
	{
		switch (key)
		{
		case GLFW_KEY_1 :
			controlerSet[3][0].pressed = false;
			break;
		case GLFW_KEY_2 :
			controlerSet[3][1].pressed = false;
			break;
		case GLFW_KEY_3 :
			controlerSet[3][2].pressed = false;
			break;
		case GLFW_KEY_4 :
			controlerSet[3][3].pressed = false;
			break;
		case GLFW_KEY_5 :
			controlerSet[3][4].pressed = false;
			break;
		}
	}
	*/
}

//------------------------------------------

SNMuseoFrei::~SNMuseoFrei()
{
}
}
