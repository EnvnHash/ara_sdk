//
//  SNSamsungColorBars.hpp
//  tav_scene
//
//  Created by Sven Hahne on 09/03/16.
//  Copyright © 2016 Sven Hahne. All rights reserved..
//

#ifndef SNSamsungColorBars_hpp
#define SNSamsungColorBars_hpp

#pragma once

#include <iostream>
#include <unistd.h>
#include <SceneNode.h>
#include <GLUtils/VAO.h>
#include "GLUtils/GWindowManager.h"
#include <GeoPrimitives/Quad.h>
#include <Communication/OSC/OSCData.h>
#include <AnimVal.h>
#include <Shaders/Shaders.h>
#include <GLUtils/TextureManager.h>

namespace tav
{
class SNSamsungColorBars: public SceneNode
{

public:
	SNSamsungColorBars(sceneData* _scd,
			std::map<std::string, float>* _sceneArgs);
	~SNSamsungColorBars();

	void initShdr(camPar* cp);
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void buildTunnelShape(camPar* cp);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;

private:
	OSCData* osc;

	Quad* quad;
	Quad* quadColorBLR;

	Quad* clearQuad;

	TextureManager* colorBar;
	TextureManager* colorBarLR;
	TextureManager* black;
	VAO* tunnelShape;

	Shaders* stdCol;
	Shaders* stdTex;
	Shaders* shdr;
	ShaderCollector* shCol;

	glm::mat4* modelMat;
	glm::mat4* rotMats;
	glm::mat4* identMat;
	camPar* camP = 0;

	AnimVal<float>* enterFromLeft;
	AnimVal<float>* scaleTex;
	AnimVal<float>* rotateQuad;
	AnimVal<float>* fromTop;
	AnimVal<float>* waitRot;
	AnimVal<float>* multQuadTrans;
	AnimVal<float>* endSwitch;
	AnimVal<float>* offTimeV;

	double enterFromLeftTime;
	double scaleTexTime;
	double waitTime;
	double rotate90Time;
	double fromTopTime;
	double waitToRotTime;
	double multQuadTransTime;
	double endSwitchTime;
	double offTime;

	double actTime;

	bool goToStart = false;

	int nrBars;

	int enterFromLeftTimeCntr = 0;

	bool inited = false;

	glm::mat4* wallMats;
	glm::mat4 calcMat;
};
}

#endif /* SNSamsungColorBars_hpp */
