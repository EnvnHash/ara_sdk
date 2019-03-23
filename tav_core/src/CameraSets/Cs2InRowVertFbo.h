//
//  Cs2InRowVertFbo.h
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma GCC visibility push(default)
#pragma once

#include <iostream>
#include <functional>

#include "CameraSet.h"
#include "GLUtils/FboDedistPersp.h"
#include "GLUtils/GWindowManager.h"

namespace tav
{
class Cs2InRowVertFbo: public CameraSet
{
public:
	enum drType
	{
		DRAW = 0, TESTPIC = 1
	};
	Cs2InRowVertFbo(sceneData* _scd, OSCData* _osc, GWindowManager* _winMan);
	~Cs2InRowVertFbo();
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
	FboDedistPersp* fboDedist;
	std::function<void()> setupFunc;
	GWindowManager* winMan;
	Shaders* texShdr;
	float* rotAngle;
	glm::vec3* camPos;
	glm::vec3* rotAxis;
	drType renderMode;

	bool lightNotInit = true;
};
}

#pragma GCC visibility pop
