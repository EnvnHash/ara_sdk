//
// SNTestParticlesFboSpriteTex.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <Shaders/ShaderCollector.h>
#include <GLUtils/GLSL/GLSLParticleSystemFbo.h>
#include <SceneNode.h>

namespace tav
{

class SNTestParticlesFboSpriteTex : public SceneNode
{
public:
	SNTestParticlesFboSpriteTex(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestParticlesFboSpriteTex();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void initSpriteShader();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);
private:
	GLSLParticleSystemFbo*          ps;
	GLSLParticleSystemFbo::EmitData data;
	GLuint*                         textures;

	Quad*							quad;

	Shaders*                        spriteShader;
	Shaders*                        stdTex;

	ShaderCollector*				shCol;

	TextureManager*                 emitTex;
	TextureManager*                 partTex;
	TextureManager*                 partTexNorm;
	TextureManager*                 litsphereTexture;

	float                           texNrElemStep;
	float                           elemPerSide;
	int                             texElementsPerSide;

	double                          intrv;
	double                          lastTime=0.0;

	glm::vec4                       diffuse;
	glm::vec4                       specular;
	glm::vec3                       lightPos;
};
}
