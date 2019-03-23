//
// SNGam_Musica_Fondo.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <unistd.h>
#include <iostream>

#include <opencv2/core.hpp>

#include <SceneNode.h>
#include "GeoPrimitives/QuadArray.h"
#include "GeoPrimitives/Quad.h"
#include <PAudio.h>
#include "GLUtils/PingPongFbo.h"
#include "Shaders/ShaderCollector.h"
#include "GeoPrimitives/Line.h"
#include "Communication/OSC/OSCData.h"

namespace tav
{
class SNGam_Musica_Fondo: public SceneNode
{
public:
	SNGam_Musica_Fondo(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNGam_Musica_Fondo();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void initPreCalcShdr();
	void initWaveShdr(TFO* _tfo);
	void initRenderShdr();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods) {}

	void loadCalib();
	void saveCalib();

private:
	bool inited = false;

	OSCData* osc;
	PAudio* pa;
	Quad* quad;
	QuadArray* quadAr;

	ShaderCollector* shCol;

	Shaders* posTexShdr;
	Shaders* normTexShdr;
	Shaders* renderShdr;

	TextureManager* litsphereTex;
	TextureManager* bumpMap;
	TextureManager* cubeTex;

	PingPongFbo* posTex;
	FBO* normTex;

	double lastTime;
	double intTime = 0;

	float aux1 = 0.f;
	float aux2 = 0.f;
	float alpha = 1.f;
	float reflAmt = 1.f;
	float brightAdj = 1.f;
	float heightScale = 1.f;
	float distAmp;
	float audioDistMed;
	float lineThick;
	float nrLines;
	float ringFadeOffs;
	float ringFade;

	int lastBlock = -1;
	int nrSegs;

	unsigned int texGridSize;
	unsigned int nrBaseCols;

	glm::vec4* baseCol;
	glm::mat4 intModelMat;
	glm::mat3 normalMat;
	glm::mat4 texBorderScaleMat;

	std::string calibFileName;
};
}
