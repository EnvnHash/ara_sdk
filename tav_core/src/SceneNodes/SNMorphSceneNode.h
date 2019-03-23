//
// SNMorphSceneNode.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include <SceneNode.h>

#include <Communication/OSC/OSCData.h>
#include <GeoPrimitives/Quad.h>
#include <GLUtils/FBO.h>
#include "GLUtils/TFO.h"
#include <Shaders/ShaderCollector.h>

#include <GLFW/glfw3.h>

namespace tav
{
class SNMorphSceneNode: public SceneNode
{
public:
	SNMorphSceneNode(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	//, OSCData* _osc, TFO** _tfos, scBlendData* _scbData, float* _modelM, float* _viewM, float* _projM);
	~SNMorphSceneNode();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);
	void setSnapShotMode(bool _val);
	void setupMorphShdr();
private:
	GLuint* resVAO;
	TFO** geoRecorders;
	FBO* snapShotFbo;
	Shaders* morphShader;
	scBlendData* scbData;
	OSCData* osc;
	ShaderCollector* shCol;

	Quad* quad;
	Quad* texQuad;

	float* modelM;
	float* viewM;
	float* projM;

	int* texNrs;
	float** auxCol;

	bool reqSnapshot;
	bool snapShotMode = false;

	int snapWidth;
	int snapHeight;
	int savedNr = 0;
};
}
