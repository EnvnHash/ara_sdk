//
//  SHShadow.h
//  tav_gl4
//
//  Created by Sven Hahne on 17.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <cmath>
#include "Shaders/ShadowMaps/ShadowMap.h"

namespace tav
{
class ShadowMapEsm: public ShadowMap
{
public:
	ShadowMapEsm(camPar* _cp, int _scrWidth, int _scrHeight,
			glm::vec3 _lightPos, float _near, float _far,
			ShaderCollector* _shCol);
	~ShadowMapEsm();

	//void            render(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	void setLightPos(glm::vec3 _pos);

	float getCoef();
	GLenum getColorBufType();
private:
	GLMCamera* lightCam;
	GLenum colBufType;
};
}
