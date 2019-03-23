//
// SNTestPartQuadBlend.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestPartQuadBlend.h"

using namespace glm;

namespace tav
{
SNTestPartQuadBlend::SNTestPartQuadBlend(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	ps = new GLSLParticleSystem(shCol, 100, scd->screenWidth, scd->screenHeight);
	ps->setFriction(0.3f);
	ps->setLifeTime(1.f);
	ps->setAgeFading(true);
}

//----------------------------------------------------

void SNTestPartQuadBlend::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if(!isInited)
	{
		ps->init(_tfo);
		isInited = true;
	}

	// das partikel system verwendet einen eigenen
	// internen shader zum geometrie aufnehmen
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		for (auto i=0;i<4;i++)
			useTextureUnitInd(i, textures[i], _shader, _tfo); // zum tfo aufnehmen der texturen
		ps->drawToBlend(GLSLParticleSystem::QUADS, _tfo);

	} else
	{
		ps->bindStdShader(GLSLParticleSystem::QUADS);
		for (auto i=0;i<8;i++)
		{
			glActiveTexture(GL_TEXTURE0+i);
			glBindTexture(GL_TEXTURE_2D, textures[i]);
		}
		ps->draw();
	}
}

//----------------------------------------------------

void SNTestPartQuadBlend::update(double time, double dt)
{
	// das partikelsystem verwendet interne tfos...
	data.emitOrg = glm::vec3(0.f, 0.f, 0.f);
	data.emitVel = normalize(glm::vec3(-0.1f, -0.0f, 0.f));
	data.speed = 1.0f;
	data.size = 0.05f;
	data.dirRand = 0.8f;
	data.texUnit = 1;
	data.sizeRand = 0.8f;
	data.angleRand = 0.2f;
	data.lifeRand = 0.2f;
	data.texRand = 1.f;

	ps->emit(1, data);
	ps->update(time);
}

//----------------------------------------------------

SNTestPartQuadBlend::~SNTestPartQuadBlend()
{
	delete ps;
}
}
