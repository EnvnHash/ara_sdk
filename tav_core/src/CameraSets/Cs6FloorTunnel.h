//
//  Cs6FloorTunnel.h
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
class Cs6FloorTunnel: public CameraSet
{
public:
	Cs6FloorTunnel(sceneData* _scd, OSCData* _osc, GWindowManager* _winMan);
	~Cs6FloorTunnel();
	void initShader();
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
	FboDedistPersp* fboDedist;
	std::function<void()> setupFunc;
	GWindowManager* winMan;
	Shaders* texShdr;
	TextureManager* floorTex;
	Quad** tunVisQuads;
	GLMCamera* tunVisCam;
	float* rotAngle;
	glm::vec3* camPos;
	glm::vec3* rotAxis;

	bool lightNotInit = true;
	bool renderVisu = false;

	int destWidth = 1280;
	int destHeight = 720;

	int floorWidth = 1280;
	int floorHeight = 720;
};
}

#pragma GCC visibility pop
