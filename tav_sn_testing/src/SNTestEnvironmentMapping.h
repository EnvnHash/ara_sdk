//
// SNTestEnvironmentMapping.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include <Shaders/Shaders.h>
#include <GLUtils/SkyBox.h>
#include <GeoPrimitives/Sphere.h>
#include <GLUtils/GLMCamera.h>

namespace tav
{

class SNTestEnvironmentMapping : public SceneNode
{
public:
	enum envMapType { ONLY_ENV=0, ENV_REFR=1 };
	SNTestEnvironmentMapping(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestEnvironmentMapping();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};

private:
	Quad*               quad;
	ShaderCollector*				shCol;
	Shaders*            envShdr;
	SkyBox*             skyBox;
	Sphere*             sphere;
	TextureManager*     cubeTex;
	TextureManager*     bumpTex;
	GLMCamera*          cam;

	std::string*        dataPath;
	envMapType          actMode;
};
}
