//
//  Cs1PerspFboFft.h
//  tav_gl4
//
//  Created by Sven Hahne on 22.12.15.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma GCC visibility push(default)

#pragma once

#include <iosfwd>
#include <functional>

#include "CameraSet.h"
#include "GLUtils/FboDedistPersp.h"
#include "GLUtils/GWindowManager.h"
#include "FFT.h"
#include "Shaders/Shaders.h"
#include "Communication/OSC/OSCHandler.h"
#include "GLUtils/PingPongFbo.h"

namespace tav
{
class Cs1PerspFboFft: public CameraSet
{
public:
	Cs1PerspFboFft(sceneData* _scd, OSCData* _osc, std::vector<fboView*>* _fboViews, GWindowManager* _winMan,
			OSCHandler* _osc_handler);
	~Cs1PerspFboFft();

	void initDiffShdr();
	void initProto(ShaderProto* _proto);
	void render(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	void preDisp(double time, double dt);
	void postRender();
	void renderFbos(double time, double dt, unsigned int ctxNr);

	glm::vec3 RGBtoHSV(float r, float g, float b);

	// void setLightProto(string _protoName);
	void onKey(int key, int scancode, int action, int mods);
	void mouseBut(GLFWwindow* window, int button, int action, int mods);
	void mouseCursor(GLFWwindow* window, double xpos, double ypos);
	void clearFbo();
private:
	std::function<void()> setupFunc;
	FboDedistPersp* fboDedist;
	GWindowManager* winMan;

	OSCHandler* osc_handler;

	Shaders* stdTex;
	Shaders* diffShdr;
	PingPongFbo* renderFbo;
	FBO* diffFbo;
	FFT* fft;

	int fftSize;
	int sendInt;
	int sendCtr;
	int gridRowSize;
	int gridSize;

	float gridStep;
	float halfGridStep;

	float* gridVals;
	float* outAmp;
	float* outHue;
	unsigned char* data;
	Median<float>** medAmps;
	Median<float>** medHues;
};
}

#pragma GCC visibility pop
