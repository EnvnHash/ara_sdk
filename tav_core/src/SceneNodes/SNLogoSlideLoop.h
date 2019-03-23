//
//  SNLogoSlideLoop.hpp
//  tav_scene
//
//  Created by Sven Hahne on 09/03/16.
//  Copyright © 2016 Sven Hahne. All rights reserved..
//

#ifndef SNLogoSlideLoop_hpp
#define SNLogoSlideLoop_hpp

#pragma once

#include <iostream>

#include <SceneNode.h>

#include <Communication/OSC/OSCData.h>
#include <GeoPrimitives/Quad.h>
#include "GLUtils/GWindowManager.h"
#include <GLUtils/TextureManager.h>
#include <GLUtils/VAO.h>
#include <Shaders/Shaders.h>
#include <AnimVal.h>

namespace tav
{
class SNLogoSlideLoop: public SceneNode
{

public:
	SNLogoSlideLoop(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNLogoSlideLoop();

	void initShdr(camPar* cp);
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void buildTunnelShape(camPar* cp);
	void startLogoSlide();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;

private:
	OSCData* osc;
	Quad* quad;
	TextureManager* samsLogo;
	TextureManager* samsLogoWhite;
	VAO* tunnelShape;

	Shaders* stdCol;
	Shaders* stdTex;
	Shaders* shdr;
	ShaderCollector* shCol;

	glm::mat4 modelMat;
	glm::mat4* identMat;
	glm::vec3 roomDim;

	camPar* camP = 0;
	AnimVal<float>* logoPos;
	AnimVal<float>* waitForActivation;
	AnimVal<float>* blendToWhite;

	float logoScale;
	float scaleFact;

	double waitTime;
	double blendTime;
	double slideTime;

	double actTime;

	bool inited = false;
};
}

#endif /* SNLogoSlideLoop_hpp */
