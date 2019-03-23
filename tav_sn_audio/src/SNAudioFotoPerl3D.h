//
// SNAudioFotoPerl3D.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2016 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include "GLUtils/GWindowManager.h"
#include <PAudio.h>
#include "Communication/OSC/OSCData.h"
#include "GeoPrimitives/QuadArray.h"
#include "GLUtils/TextureManager.h"
#include "GLUtils/Noise3DTexGen.h"

namespace tav
{
class SNAudioFotoPerl3D: public SceneNode
{
public:
	SNAudioFotoPerl3D(sceneData* _scd,
			std::map<std::string, float>* _sceneArgs);
	~SNAudioFotoPerl3D();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);

private:
	PAudio* pa;
	OSCData* osc;
	ShaderCollector* shCol;

	QuadArray** quadAr;
	TextureManager** img;
	Noise3DTexGen* noiseTex;

	glm::vec4* audioCol;
	glm::vec4* chanCols;

	bool inited = false;

	int blockInd = -1;
	int numDiffTex;

	float audioTex2DNrChans;
	float alpha = 0.f;
	float steady = 0.f;
	float planeOffs = 0.1f;
	float planeZRot = 0.f;
	float toRad = 0.4f;
};
}
