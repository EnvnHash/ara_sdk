//
// SNMuseoFrei.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <mutex>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <limits>

#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include <FFMpegDecode.h>
#include <GLUtils/TextureManager.h>
#include <GLUtils/GWindowManager.h>
#include <AnimVal.h>

#include <GLFW/glfw3.h>


namespace tav
{
class SNMuseoFrei: public SceneNode
{
public:
	SNMuseoFrei(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNMuseoFrei();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void trig_video_endCb();
	void onKey(int key, int scancode, int action, int mods);

private:

	AnimVal<float>*			blendVal;

	FFMpegDecode			loop;
	FFMpegDecode*			trig_video;
	GWindowManager*			winMan;
	Quad*					quad;
	Shaders*				stdTex;
	ShaderCollector*		shCol;

	bool					isInited = false;
	bool					requestTrigVideo;
	bool					requestLoopVideo;
	bool					isBlending;
	bool					debug;

	double					debugSwitchTime;
	double					lastDebugAct;
	double					blendTime;
};
}
