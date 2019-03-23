//
//  SPShadow.h
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include "ShaderProto.h"
#include "../Shaders/ShadowMaps/ShadowMapStd.h"
#include "../Shaders/ShaderUtils/LightProperties.h"

namespace tav
{
class SPShadow: public ShaderProto
{
public:
	SPShadow(spData* _sData, ShaderCollector* _shCol);
	~SPShadow();
	void defineVert(bool _multiCam);
	void defineGeom(bool _multiCam);
	void defineFrag(bool _multiCam);
	void preRender(SceneNode* _scene, double time, double dt);
	void sendPar(int nrCams, camPar& cp, OSCData* osc, double time);
private:
	glm::mat4 scale_bias_matrix;
	glm::mat4 shadow_matrix;
	LightProperties* lightProp;
	ShadowMap* shMap;
	bool shMapInited = false;
	int scrWidth;
	int scrHeight;
	float near;
	float far;
};
}
