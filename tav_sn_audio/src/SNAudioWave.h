//
// SNAudioWave.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include "GeoPrimitives/Line.h"
#include <PAudio.h>

namespace tav
{
class SNAudioWave: public SceneNode
{
public:
	SNAudioWave(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNAudioWave();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;
	void updateWave();
private:
	ShaderCollector* shCol;
	PAudio* pa;
	Line** lines;
	int lastBlock = -1;

	float* pllOffs;
	float* signals;
	float ampScale;
	float posOffs;

	int nrPar;
	glm::vec4* chanCols;

	Shaders* colShader;
};
}
