//
// SNImageFFT.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <Communication/OSC/OSCHandler.h>
#include <GeoPrimitives/Quad.h>
#include <GLUtils/FBO.h>
#include <GLUtils/TextureManager.h>
#include <SceneNode.h>

#ifdef WITH_AUDIO
#include <FFT.h>
#endif

namespace tav
{
class SNImageFFT: public SceneNode
{
public:
	SNImageFFT(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNImageFFT();

	void initShdr();
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods) {}

private:
	Quad* quad;

	Shaders* stdTex;
	Shaders* testShdr;
	ShaderCollector* shCol;

	FBO* renderFbo;

#ifdef WITH_AUDIO
	FFT* fft;
#endif

	OSCHandler* osc_handler;
	TextureManager* tex;

	int fftSize;
	int sendInt;
	int sendCtr;

	float* oneRow;
	float* newAmp;

	unsigned char* data;
};
}
