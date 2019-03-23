//
//  SNExplodeGeo.hpp
//  tav_scene
//
//  Created by Sven Hahne on 09/03/16.
//  Copyright © 2016 Sven Hahne. All rights reserved..
//

#ifndef SNExplodeGeo_hpp
#define SNExplodeGeo_hpp

#pragma once

#include <iostream>

#include <SceneNode.h>

#include <Communication/OSC/OSCData.h>
#include <GeoPrimitives/QuadArray.h>
#include <GLUtils/GWindowManager.h>
#include "GLUtils/Noise3DTexGen.h"
#include <AnimVal.h>

namespace tav
{
class SNExplodeGeo: public SceneNode
{

public:
	SNExplodeGeo(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNExplodeGeo();

	void initShdr(camPar* cp);
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void restartMorph();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{}

private:
	OSCData* osc;
	QuadArray* quadAr;
	TextureManager* objTex;
	Shaders* explShdr;
	Noise3DTexGen* noiseTex;
	ShaderCollector* shCol;

	AnimVal<float>* explSlid;
	AnimVal<float>* standby;

	float objSize;
	bool inited = false;

	double actTime;
};
}

#endif /* SNExplodeGeo_hpp */
