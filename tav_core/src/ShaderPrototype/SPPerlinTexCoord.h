//
//  SPPerlinTexCoord.h
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include "ShaderProto.h"
#include "../Shaders/ShaderUtils/LightProperties.h"
#include <math.h>       /* modf */

namespace tav
{
class SPPerlinTexCoord: public ShaderProto
{
public:
	SPPerlinTexCoord(spData* _sData, ShaderCollector* _shCol);
	~SPPerlinTexCoord();
	void defineVert(bool _multiCam);
	void defineGeom(bool _multiCam);
	void defineFrag(bool _multiCam);
	void preRender(SceneNode* _scene, double time, double dt);
	void sendPar(int nrCams, camPar& cp, OSCData* osc, double time);
private:
	glm::vec3 lightDir;
	glm::vec3 halfVector;
	LightProperties* lightProp;
};
}
