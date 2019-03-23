//
//  Cs5Tunnel.h
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
class Cs5Tunnel: public CameraSet
{
public:
	enum drType
	{
		DRAW = 0, VISU = 1, TESTPIC = 2
	};
	Cs5Tunnel(sceneData* _scd, OSCData* _osc, GWindowManager* _winMan);
	~Cs5Tunnel();
	void initShader();
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
	TextureManager* patron;
	Quad* patronQuad;
	Quad** tunVisQuads;
	GLMCamera* tunVisCam;
	float* rotAngle;
	glm::vec3* camPos;
	glm::vec3* rotAxis;
	drType renderMode;

	bool lightNotInit = true;
	bool renderVisu = false;

	float tunnelDepth;	// referenz = 2.f
	float tunnelHeight;
	float tunnelWidth;
};
}

#pragma GCC visibility pop
