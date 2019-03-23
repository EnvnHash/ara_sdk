//
// SNVideoTileAudio3d.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GeoPrimitives/QuadArray.h>
#include <Communication/OSC/OSCData.h>
#include <VideoTextureCv.h>
#include <PAudio.h>

namespace tav
{
class SNVideoTileAudio3d: public SceneNode
{
public:
	SNVideoTileAudio3d(sceneData* _scd,
			std::map<std::string, float>* _sceneArgs);
	~SNVideoTileAudio3d();
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;
	void updateModMatr(float dt);
private:
	PAudio* pa;
	OSCData* osc;
	QuadArray* quad;
	VideoTextureCv* vt;
	ShaderCollector* shCol;
	bool isInited = false;
	int lastBlock;
	unsigned int nrInstX;
	unsigned int nrInstY;
	unsigned int nrInstZ;

	glm::vec4* chanCols;

	glm::vec3 fNrInst;
	glm::vec3 stepSize;
	float* transInd;

	float pllDirCalcOffs;
	unsigned int nrTexOffPos;
	glm::vec2** texOffDir;
	glm::vec2** texOffPos;
	float texOffsSpeed;
	float texOffsMed;
	float texOffsPosMed;

	unsigned int nrRandTable;
	float** randTable;
	int nrTransInd;
};
}
