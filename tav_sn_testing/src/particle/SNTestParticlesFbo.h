//
// SNTestParticlesFbo.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <Shaders/ShaderCollector.h>
#include <GLUtils/GLSL/GLSLParticleSystemFbo.h>
#include <Shaders/Shaders.h>
#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include <GLUtils/TextureManager.h>

namespace tav
{

class SNTestParticlesFbo : public SceneNode
{
public:
	SNTestParticlesFbo(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestParticlesFbo();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);
private:
	GLSLParticleSystemFbo*          ps;
	GLSLParticleSystemFbo::EmitData data;
	ShaderCollector*				shCol;

	Shaders*                        stdTexShader;
	Quad*                           quad;
	TextureManager*                 emitTex;

	unsigned int                    nrTestPart;
	double                          lastTime = 0.0;
	double                          intrv;
};

}
