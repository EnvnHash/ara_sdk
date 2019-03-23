//
//  Cs4Cube.h
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma GCC visibility push(default)
#pragma once

#include <iostream>
#include "CameraSet.h"

namespace tav
{
class Cs4Cube: public CameraSet
{
public:
	Cs4Cube(sceneData* _scd, OSCData* _osc);
	~Cs4Cube();
	void initProto(ShaderProto* _proto);
	//void clearScreen();
	void render(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	void preDisp(double time, double dt);
	void postRender();
	void renderFbos(double time, double dt, unsigned int ctxNr);
	void onKey(int key, int scancode, int action, int mods);
	void mouseBut(GLFWwindow* window, int button, int action, int mods);
	void mouseCursor(GLFWwindow* window, double xpos, double ypos);
	void clearFbo();

private:
	float overlap = 0.f;
	bool lightNotInit = true;
};
}

#pragma GCC visibility pop
