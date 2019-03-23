//
//  Cs1PerspFboBack.h
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

namespace tav
{
class Cs1PerspFboBack: public CameraSet
{
public:
	Cs1PerspFboBack(sceneData* _scd, OSCData* _osc, std::vector<fboView*>* _fboViews, GWindowManager* _winMan);
	~Cs1PerspFboBack();
	void initProto(ShaderProto* _proto);
	void render(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	void preDisp(double time, double dt);
	void postRender();
	void renderFbos(double time, double dt, unsigned int ctxNr);

	// void setLightProto(string _protoName);
	void onKey(int key, int scancode, int action, int mods);
	void mouseBut(GLFWwindow* window, int button, int action, int mods);
	void mouseCursor(GLFWwindow* window, double xpos, double ypos);
	void clearFbo();
private:
	std::function<void()> setupFunc;
	FboDedistPersp* fboDedist;
	GWindowManager* winMan;
	Shaders* stdTex;
};
}

#pragma GCC visibility pop
