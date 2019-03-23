//
//  Cs1FboAspect.h
//  tav_gl4
//
//  Created by Sven Hahne on 22.12.15.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iosfwd>
#include <functional>

#include "headers/opencv_headers.h"
#include "CameraSet.h"
#include "GLUtils/FboDedistPersp.h"
#include "GLUtils/GWindowManager.h"

namespace tav
{
class Cs1FboAspect: public CameraSet
{
public:
	Cs1FboAspect(sceneData* _scd, OSCData* _osc, std::vector<fboView*>* _fboViews, GWindowManager* _winMan);
	~Cs1FboAspect();

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

	// overwrite CameraSet clearscreen, since this set is fbo based, and there could be
	// more than one cameraset, don't use the standard clear function
	void clearScreen(){ }

private:
	std::function<void()> setupFunc;
	FboDedistPersp* fboDedist;
	GWindowManager* winMan;
	int savedNr = 0;
	bool reqSnapshot = false;
	glm::vec2 fboSize;
};
}
