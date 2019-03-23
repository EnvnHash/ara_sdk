//
// SNTestParticlesFbo.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  die "update" methode muss aufgerufen werden!!!

#include "SNTestParticlesFbo.h"

using namespace glm;

namespace tav
{
SNTestParticlesFbo::SNTestParticlesFbo(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	intrv = 0.5;
	nrTestPart = 10000;
	ps = new GLSLParticleSystemFbo(shCol, nrTestPart,
			float(_scd->screenWidth) / float(_scd->screenHeight));
	ps->setFriction(0.f);
	ps->setLifeTime(8.f);
	ps->setAging(true);
	ps->setAgeFading(true);

	// EmitData muss explizit geschickt werden, dadurch Optimierung mit UniformBlocks möglich
	data.emitVel = normalize(glm::vec3(0.f, 1.f, 0.f));
	data.emitCol = glm::vec4(1.f, 1.f, 1.f, 1.f);
	data.posRand = 0.01f; // wenn emit per textur und > 0 -> mehr gpu
	data.dirRand = 0.1f;
	data.speed = 0.05f;
	data.ageFadeCurv = 0.2f;
	ps->setEmitData(&data);

	emitTex = new TextureManager();
	emitTex->loadTexture2D((*_scd->dataPath)+"/textures/test_emit2.tif");

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	stdTexShader = shCol->getStdTex();
}

//----------------------------------------------------

void SNTestParticlesFbo::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	ps->draw( cp->mvp );
}

//----------------------------------------------------

void SNTestParticlesFbo::update(double time, double dt)
{
	int nrEmit = 90;
	if (time - lastTime > intrv)
	{
		// emit with texture
		ps->emit(nrEmit, emitTex->getId(), emitTex->getWidth(), emitTex->getHeight());

		lastTime = time;
	}

	ps->update(time);
}

//----------------------------------------------------

void SNTestParticlesFbo::onKey(int key, int scancode, int action, int mods)
{}

//----------------------------------------------------

SNTestParticlesFbo::~SNTestParticlesFbo()
{
	delete ps;
	delete quad;
	delete emitTex;
}
}
