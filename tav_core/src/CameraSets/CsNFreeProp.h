//
//  CsNFreeProp.h
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma GCC visibility push(default)
#pragma once

#include "CameraSet.h"
#include <iosfwd>

namespace tav
{
class CsNFreeProp: public CameraSet
{
public:
	CsNFreeProp(sceneData* _scd, OSCData* _osc,
			std::vector<fboView*>* _screens);
	~CsNFreeProp();
	void initProto(ShaderProto* _proto);
	//void clearScreen();
	void render(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	void preDisp(double time, double dt);
	void postRender();
	void renderFbos(double time, double dt, unsigned int ctxNr);
	void onKey(int key, int scancode, int action, int mods);
	void mouseBut(GLFWwindow* window, int button, int action, int mods);
	void mouseCursor(GLFWwindow* window, double xpos, double ypos);
	void mapScreens();
	void applyMatrix(vector<fboView*>* inData, std::vector<fboView*>& outData,
			glm::mat4 transM);
	void getMinMax(glm::vec3* limits, glm::vec3* points);
	void buildCams();
	void clearFbo();
private:
	float overlap = 0.f;
	bool lightNotInit = true;
	glm::vec3 align;
	glm::vec3 camAlign;
	std::vector<fboView*> mappedScreens;
	std::vector<glm::vec3> lookAt;
	std::vector<glm::vec3> camPos;
	std::vector<fboView*>* screens;
};
}

#pragma GCC visibility pop
