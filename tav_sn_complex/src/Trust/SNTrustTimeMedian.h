//
// SNTrustTimeMedian.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once


#include <iostream>

#include "GLUtils/GLSL/GLSLTimeMedian.h"
#include <GeoPrimitives/Quad.h>
#include <SceneNode.h>
#include <VideoTextureCvActRange.h>

namespace tav
{

class SNTrustTimeMedian : public SceneNode
{
public:
	SNTrustTimeMedian(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTrustTimeMedian();

	void init(TFO* _tfo = nullptr);
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void startThreads(double time, double dt);
	void stopThreads(double time, double dt);
	void cleanUp();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};

private:
    GLSLTimeMedian*			tMed;
	Quad*                   quad;
    ShaderCollector*		shCol;
	Shaders*            	stdColShdr;
	Shaders*            	stdTex;
	VideoTextureCvActRange* vt;

	int						actUplTexId;
	int						lastTexId;

	float					alpha=0.f;
	float					median=136.f;
};
}
