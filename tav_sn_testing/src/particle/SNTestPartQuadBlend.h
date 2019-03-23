//
// SNTestPartQuadBlend.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <Shaders/ShaderCollector.h>
#include <GLUtils/GLSL/GLSLParticleSystem.h>
#include <SceneNode.h>

namespace tav
{

class SNTestPartQuadBlend : public SceneNode
{
public:
	SNTestPartQuadBlend(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestPartQuadBlend();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};

private:
	ShaderCollector*				shCol;
	GLSLParticleSystem*             ps;
	GLSLParticleSystem::EmitData    data;
	GLuint*                         textures;
	bool                            isInited=false;
};

}
