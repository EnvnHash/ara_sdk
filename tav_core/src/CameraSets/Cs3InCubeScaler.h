//
//  Cs3InCubeScaler.h
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma GCC visibility push(default)

#pragma once

#include <iosfwd>
#include "CameraSet.h"

namespace tav
{
class Cs3InCubeScaler: public CameraSet
{
public:
	Cs3InCubeScaler(sceneData* _scd, OSCData* _osc);
	~Cs3InCubeScaler();
	void initProto(ShaderProto* _proto);
	void clearFbo();
	void render(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	void preDisp(double time, double dt);
	void postRender();
	void renderFbos(double time, double dt, unsigned int ctxNr);

	// void setLightProto(string _protoName);
	void onKey(int key, int scancode, int action, int mods);
	void mouseBut(GLFWwindow* window, int button, int action, int mods);
	void mouseCursor(GLFWwindow* window, double xpos, double ypos);

private:
	Shaders* mappingShader;
	glm::vec2 leftScreenDim;
	glm::vec2 centerScreenDim;
	glm::vec2 rightScreenDim;

	glm::mat4* modMatrices;
	glm::mat4* viewMatrices;
	glm::mat4 projMatrices;
};
}

#pragma GCC visibility pop
