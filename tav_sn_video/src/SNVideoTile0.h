//
// SNVideoTile0.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GeoPrimitives/QuadArray.h>
#include <VideoTextureCv.h>
#include <Communication/OSC/OSCData.h>

namespace tav
{
class SNVideoTile0: public SceneNode
{
public:
	SNVideoTile0(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNVideoTile0();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods) {}
	void updateModMatr(float dt);
private:
	OSCData* osc;
	QuadArray* quad;
#ifdef HAVE_OPENCV
	VideoTextureCv* vt;
#endif
	ShaderCollector* shCol;
	bool isInited = false;
	int lastBlock;
	int nrInstances;
	float fNrInst;
	float tileSize;
	float stepSize;
};
}
