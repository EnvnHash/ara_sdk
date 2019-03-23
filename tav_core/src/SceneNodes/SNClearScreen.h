//
// SNClearScreen.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include <Communication/OSC/OSCData.h>

namespace tav
{
class SNClearScreen: public SceneNode
{
public:
	SNClearScreen(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNClearScreen();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;
private:
	OSCData* osc;
	Shaders* clearShader;
	glm::vec4 clearCol;
	ShaderCollector* shCol;
};
}
