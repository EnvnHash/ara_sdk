//
//  Cs3InRow.h
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma GCC visibility push(default)
#pragma once

#include <iosfwd>
#include "CameraSet.h"

#include <GLFW/glfw3.h>

namespace tav
{
class Cs3InRow: public CameraSet
{
public:
	Cs3InRow(sceneData* _scd, OSCData* _osc);
	~Cs3InRow();
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
	bool ightNotInit = true;
};
}

#pragma GCC visibility pop
