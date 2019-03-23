/*
 *  OSCData.h
 *  ta_visualizer
 *
 *  Created by Sven Hahne on 06.01.11.
 *  Copyright 2011 Sven Hahne. All rights reserved.
 *
 */

#pragma once

#include <vector>
#include <math.h>
#include <iostream>
#include <limits>
#include <map>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "../../SceneNode.h"
#include "headers/gl_header.h"
#include "headers/global_vars.h"

using std::vector;

namespace tav
{
class OSCData
{
public:
	enum contrShape { LIN, EXP };
	typedef struct
	{
		float min;
		float max;
		float step;
		float initVal;
		contrShape shape;
		float val;
	} contrSpec;

	OSCData();
	~OSCData();

	void getNewVals(double dt);
	void addPar(std::string _name, float _min, float _max, float _step,
			float _initVal, contrShape _shape);
	float getPar(std::string _name);

	float seqParMed;
	float parMed;
	std::vector<float> osc_vals;
	std::vector<float> osc_in_vals;

	std::map<std::string, contrSpec> nodePar;
	std::map<const char*, std::map<const char*, float> > nodeOscPar;

	float scBlend = 0.f;
	int sceneNum1 = 0;
	int sceneNum2 = 1;
	float audioSmooth = 0.5f;
	float totalBrightness = 1.f;
	float feedback = 0.f;
	float blurOffs = 0.f;
	float blurFboAlpha = 0.f;
	float blurFdbk = 0.f;
	float alpha = 1.f;
	float rotYAxis = 0.f;
	float speed = 1.f;
	float backColor = 0.f;
	float startStopVideo = 1.f;
	float videoSpeed = 1.f;
	float zoom = 1.f;
	float sliderX = 0.f;
	float sliderY = 0.f;

	int extStart = 0;

	void initData();

	bool sliderHasNewVal = false;
	bool extStartHasNewVal = false;
	bool handlerFirstRun;
	bool firstRun;
	bool inited;
	double actTime;
	double lastTime;

	std::map<std::string, SceneNode*>* sceneMap;
	std::vector<fboView*>* fboViews;
	GLFWmonitor** monitors;
	int nrMonitors;
	int ctxWidth = 0;
	int ctxHeight = 0;
	int ctxXpos = 0;
	int ctxYpos = 0;
	int fboWidth = 0;
	int fboHeight = 0;

	//  Sequencer* 	seq;
};

extern OSCData theOscData;
}
;
