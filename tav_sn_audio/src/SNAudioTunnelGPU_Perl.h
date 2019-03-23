//
// SNAudioTunnelGPU_Perl.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include "GeoPrimitives/QuadArray.h"
#include "GeoPrimitives/Quad.h"
#include <PAudio.h>
#include "GLUtils/FBO.h"
#include "GLUtils/PingPongFbo.h"
#include "Shaders/ShaderCollector.h"
#include "GeoPrimitives/Line.h"
#include "GLUtils/TextureManager.h"
#include "GLUtils/Noise3DTexGen.h"
#include "Communication/OSC/OSCData.h"

namespace tav
{
class SNAudioTunnelGPU_Perl: public SceneNode
{
public:
	SNAudioTunnelGPU_Perl(sceneData* _scd,
			std::map<std::string, float>* _sceneArgs);
	~SNAudioTunnelGPU_Perl();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void initPreCalcShdr();
	void initWaveShdr(TFO* _tfo);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;
private:
	bool inited = false;

	OSCData* osc;
	PAudio* pa;
	Quad* quad;
	QuadArray* quadAr;
	TextureManager* img;
	Noise3DTexGen* noiseTex;

	ShaderCollector* shCol;
	Shaders* posTexShdr;
	Shaders* normTexShdr;
	Shaders* waveShdr;

	FBO* posTex;
	FBO* normTex;

	double lastTime;
	double intTime = 0;
	int lastBlock = -1;
	int nrSegs;
	unsigned int texGridSize;

	glm::vec4* chanCols;
	glm::mat4 modelMat;
	glm::mat3 normalMat;
	glm::mat4 texBorderScaleMat;
};
}
