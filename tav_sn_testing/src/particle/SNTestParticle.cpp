//
// SNTestParticle.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  die "update" methode muss aufgerufen werden!!!

#include "SNTestParticle.h"

using namespace glm;

namespace tav
{

SNTestParticle::SNTestParticle(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	nrTestPart = 16;

	ps = new GLSLParticleSystem(shCol, nrTestPart, _scd->screenWidth, _scd->screenHeight);
	ps->setFriction(0.3f);
	ps->setLifeTime(4.f);
	ps->init();

	intrv = 4.0;
}

//----------------------------------------------------

void SNTestParticle::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

//----------------------------------------------------

void SNTestParticle::update(double time, double dt)
{
	if (time - lastTime > intrv)
	{
		for (auto i=0;i<nrTestPart;i++)
		{
			data.emitOrg = glm::vec3((((float)i + .5f) / (float)nrTestPart) * 2.f -1.f, 0.f, 0.f);
			data.emitVel = normalize(glm::vec3(0.0f, 1.0f, 0.f));
			data.speed = 0.1f;

			ps->emit(1, data);
		}

		lastTime = time;
	}

	ps->update(time, true);
}

//----------------------------------------------------

void SNTestParticle::onKey(int key, int scancode, int action, int mods)
{}

//----------------------------------------------------

SNTestParticle::~SNTestParticle()
{
	delete ps;
}
}
