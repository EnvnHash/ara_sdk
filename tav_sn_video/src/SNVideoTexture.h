//
// SNVideoTexture.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include "GeoPrimitives/Quad.h"
#include <GeoPrimitives/QuadArray.h>
#include <VideoTexture.h>

#ifdef HAVE_OPENCV
#include <VideoTextureCv.h>
#endif

#include <Communication/OSC/OSCData.h>

namespace tav
{
class SNVideoTexture: public SceneNode
{
public:
	SNVideoTexture(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNVideoTexture();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods) {}
private:
	OSCData* osc;
	QuadArray* quad;
#ifdef HAVE_OPENCV
	VideoTextureCv* vt;
#endif
	Shaders* stdTex;
	ShaderCollector* shCol;

	bool isInited = false;
	float alpha = 0.f;
};
}
