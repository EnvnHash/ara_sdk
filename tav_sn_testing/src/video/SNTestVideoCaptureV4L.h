//
// SNTestVideoCaptureV4L.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include "V4L/V4L.h"
#include <Shaders/ShaderCollector.h>

namespace tav
{

class SNTestVideoCaptureV4L : public SceneNode
{
public:
	SNTestVideoCaptureV4L(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestVideoCaptureV4L();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};
private:
	Quad*   	quad;
	V4L*		vt;
	bool    	isInited = false;

	float		brightness;
	float 		lastBrightness;
	float		gain;
	float 		lastGain;
	float 		saturation;
	float 		lastSaturation;
	float 		hue;
	float 		lastHue;
};
}
