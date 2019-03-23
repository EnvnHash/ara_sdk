//
//  Cs2HeadHead.h
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma GCC visibility push(default)
#pragma once

#include <iostream>
#include "CameraSet.h"
#include <GLFW/glfw3.h>

namespace tav
{
class Cs2HeadHead: public CameraSet
{
public:
	Cs2HeadHead(sceneData* _scd, OSCData* _osc);
	~Cs2HeadHead();
	void initProto(ShaderProto* _proto);
	void clearFbo();
	void render(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	void preDisp(double time, double dt);
	void postRender();
	void renderFbos(double time, double dt, unsigned int ctxNr);
	void onKey(int key, int scancode, int action, int mods);
	void mouseBut(GLFWwindow* window, int button, int action, int mods);
	void mouseCursor(GLFWwindow* window, double xpos, double ypos);
private:
	float overlap = 0.f;
	bool lightNotInit = true;
};
}

#pragma GCC visibility pop
