//
//  SPNoLight.h
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__SPNoLight__
#define __Tav_App__SPNoLight__

#pragma once

#include <stdio.h>
#include <iostream>
#include "ShaderProto.h"
#include "../Shaders/ShaderUtils/LightProperties.h"

namespace tav
{
class SPNoLight: public ShaderProto
{
public:
	SPNoLight(spData* _sData, ShaderCollector* _shCol);
	~SPNoLight();
	void defineVert(bool _multiCam);
	void defineGeom(bool _multiCam);
	void defineFrag(bool _multiCam);
	void preRender(SceneNode* _scene, double time, double dt);
	void sendPar(int nrCams, camPar& cp, OSCData* osc, double time);

};
}

#endif /* defined(__Tav_App__SPNoLight__) */
