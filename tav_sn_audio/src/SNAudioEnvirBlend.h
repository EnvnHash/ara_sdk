//
// SNAudioEnvirBlend.h
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
#include "GeoPrimitives/Sphere.h"
#include "GeoPrimitives/Quad.h"
#include <Median.h>
#include "GLUtils/Noise3DTexGen.h"
#include "Communication/OSC/OSCData.h"

namespace tav
{
class SNAudioEnvirBlend: public SceneNode
{
public:
	SNAudioEnvirBlend(sceneData* _scd,
			std::map<std::string, float>* _sceneArgs);
	~SNAudioEnvirBlend();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void initShdr(camPar* cp);
	void initRecShdr(TFO* _tfo);
	void onKey(int key, int scancode, int action, int mods);

private:
	Sphere* sphere;
	PAudio* pa;

	ShaderCollector* shCol;
	Shaders* recShdr;
	Shaders* sphereShdr;
	Shaders* stdTex;

	Quad* quad;

	OSCData* osc;

	Median<float>** rotAxisY;
	Median<float>** rotAxisZ;
	Noise3DTexGen* noiseTex;

	glm::mat4 m_pvm;
	glm::mat4** multiModelMat;
	glm::mat4* intModelMat;
	glm::mat3* normalMat;

	glm::vec4* audioCol;
	glm::vec4* chanCols;

	bool inited = false;

	int blockInd = -1;
	int audioTex2DNrChans;
	float alpha = 0.f;
};
}
