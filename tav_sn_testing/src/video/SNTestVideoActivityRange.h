//
// SNCamchSimpTextur.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <Shaders/Shaders.h>

#ifdef HAVE_OPENCV
#include <VideoTextureCv.h>
#include <VideoActivityRange.h>
#endif

namespace tav
{

class SNTestVideoActivityRange : public SceneNode
{
public:
	SNTestVideoActivityRange(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestVideoActivityRange();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};

private:
#ifdef HAVE_OPENCV
	VideoActivityRange*			vidRange;
	VideoTextureCv* 			vt;
#endif
	ShaderCollector*			shCol;

	int							actUplTexId = 0;
	int							lastTexId = -1;

	float						thres = 0.1f;
	float						median = 1.f;
	float						posColThres = 0.2f;
	float						histoSmooth = 0.f;
	float						indValThres = 0.f;
	float						valThres = 0.02f;
};
}
