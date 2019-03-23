//
//  Cs3InCubeFbo.h
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma GCC visibility push(default)
#pragma once

#include <opencv2/core.hpp>
#include "CameraSet.h"
#include "GLUtils/FBO.h"
#include "GeoPrimitives/QuadArray.h"
#include "Shaders/Shaders.h"
#include "GLUtils/GWindowManager.h"

namespace tav
{
class Cs3InCubeFbo: public CameraSet
{
public:
	Cs3InCubeFbo(sceneData* _scd, OSCData* _osc, std::vector<fboView*>* _fboViews, GWindowManager* _winMan);
	~Cs3InCubeFbo();
	void initProto(ShaderProto* _proto);
	void clearFbo();
	void render(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	void preDisp(double time, double dt);
	void postRender();
	void renderFbos(double time, double dt, unsigned int ctxNr);
	void setupRenderFboShader();
	void onKey(int key, int scancode, int action, int mods);
	void mouseBut(GLFWwindow* window, int button, int action, int mods);
	void mouseCursor(GLFWwindow* window, double xpos, double ypos);
	void loadCalib();
	void saveCalib();
private:
	float overlap = 0.f;
	float mouseX;
	float mouseY;
	bool lightNotInit = true;
	FBO* renderFbo;
	Shaders* texShader;
	GWindowManager* winMan;
	std::vector<glm::vec2> corners;
	std::vector<std::string> cornersNames;
	QuadArray* quadAr;
	std::string fileName;
};
}

#pragma GCC visibility pop
