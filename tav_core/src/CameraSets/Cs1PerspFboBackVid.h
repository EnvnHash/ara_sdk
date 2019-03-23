//
//  Cs1PerspFboBackVid.h
//  tav_gl4
//
//  Created by Sven Hahne on 22.12.15.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma GCC visibility push(default)

#pragma once

#include <iosfwd>
#include <functional>
#include <thread>
#include <mutex>

#include "CameraSet.h"
#include "GLUtils/FboDedistPersp.h"
#include "GLUtils/GWindowManager.h"
#include "../headers/opencv_headers.h"


namespace tav
{
class Cs1PerspFboBackVid: public CameraSet
{
public:
	Cs1PerspFboBackVid(sceneData* _scd, OSCData* _osc, std::vector<fboView*>* _fboViews, GWindowManager* _winMan);
	~Cs1PerspFboBackVid();
	void initProto(ShaderProto* _proto);
	void render(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	void preDisp(double time, double dt);
	void postRender();
	void renderFbos(double time, double dt, unsigned int ctxNr);
	void updateBackVid(double time);
	virtual void processQueue();

	// void setLightProto(string _protoName);
	void onKey(int key, int scancode, int action, int mods);
	void mouseBut(GLFWwindow* window, int button, int action, int mods);
	void mouseCursor(GLFWwindow* window, double xpos, double ypos);
	void clearFbo();
private:
	std::function<void()> setupFunc;

	FboDedistPersp* fboDedist;
	GWindowManager* winMan;
	TextureManager* uploadTexture;
	Shaders* stdTex;
	Quad* flipQuad;

	cv::VideoCapture* cap = 0;
	cv::Mat frame;

	bool processing = false;
	bool hasFrame = false;
	bool hasTex = false;

	double zeroTime = 0;
	unsigned int new_frame;
	unsigned int old_frame;
	std::mutex mutex;

	const char* backVidPath;

};
}

#pragma GCC visibility pop
