//
// SNAudioFluid.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNAudioFluid.h"

namespace tav
{
SNAudioFluid::SNAudioFluid(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	pa = (PAudio*) scd->pa;
	osc = (OSCData*) scd->osc;

	// setup chanCols
	getChanCols(&chanCols);

	flWidth = 400;
	flHeight = 400;

	posScale = 1.25f;
	ampScale = 12.f;

	forceScale = glm::vec2(static_cast<float>(flWidth) * 0.5f,
			static_cast<float>(flHeight) * 0.85f);
	alphaScale = 0.01f;

	fluidSim = new GLSLFluid(false, shCol);
	fluidSim->allocate(flWidth, flHeight, 1.0f);
	fluidSim->dissipation = 0.99f;
	fluidSim->velocityDissipation = 0.99f;
	fluidSim->setGravity(glm::vec2(0.0f, 0.0f));

	oldPos = new glm::vec2[scd->nrChannels];

	quadAr = new QuadArray(40, 40, -1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f));
	quad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 1.f);

	stdTexAlpha = shCol->getStdTexAlpha();

	addPar("alpha", &alpha);
	addPar("timeStep", &timeStep);
	addPar("velDiss", &velDiss);
}

//----------------------------------------------------

SNAudioFluid::~SNAudioFluid()
{
	fluidSim->cleanUp();
	delete fluidSim;
	delete quad;
}

//----------------------------------------------------

void SNAudioFluid::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);
		_tfo->setSceneNodeColors(chanCols);
	}

	glClear (GL_DEPTH_BUFFER_BIT);
	glDisable (GL_DEPTH_TEST);
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	sendStdShaderInit(_shader);
	useTextureUnitInd(0, fluidSim->getResTex(), _shader, _tfo);

	stdTexAlpha->begin();
	//stdTexAlpha->setIdentMatrix4fv("m_pvm");
	stdTexAlpha->setUniform1i("tex", 0);
	stdTexAlpha->setUniform1f("alpha", alpha * osc->alpha);
	stdTexAlpha->setUniformMatrix4fv("m_pvm", &_modelMat[0][0]);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());

	quadAr->draw(_tfo);
}

//----------------------------------------------------

void SNAudioFluid::update(double time, double dt)
{
	glm::vec2 pos, vel, c;
	glm::vec4 col;
	float amp, readPos, readPos2;

	if (lastBlock != pa->waveData.blockCounter)
	{
		readPos = (std::cos(time * 0.05f) + 1.f) * 0.5f;

		// Adding temporal Force, in pixel relative to flWidth and flHeight
		for (auto cNr = 0; cNr < scd->nrChannels; cNr++)
		{
			readPos2 = std::fmod(
					readPos
							+ (std::cos(
									time * 0.02f
											+ (float) cNr
													/ (float) scd->nrChannels)
									+ 1.f) * 0.1f, 1.f);

			pos = glm::vec2(
					std::fmax(
							std::fmin(
									(pa->getPllAtPos(scd->chanMap[cNr], readPos)
											* posScale + 1.f) * 0.5f, 0.99f),
							0.01f) * flWidth,
					std::fmax(
							std::fmin(
									(pa->getPllAtPos(scd->chanMap[cNr],
											readPos2) * posScale + 1.f) * 0.5f,
									0.99f), 0.01f) * flHeight);
			vel = (pos - oldPos[cNr]) * 4.f;
			oldPos[cNr] = pos;
			c = glm::normalize(forceScale - pos);

			col = chanCols[cNr % MAX_NUM_COL_SCENE] * osc->totalBrightness;
			amp = std::fmin(pa->getMedAmp(scd->chanMap[cNr]) * ampScale, 1.f);
			eDen = amp * 0.2f;
			eTemp = amp * 10.f;
			eRad = amp * 10.f + 0.9f;
			//col.a = amp * alphaScale;
			fluidSim->addTemporalForce(pos, vel, col, eRad, eTemp, eDen);
			fluidSim->setTimeStep(timeStep);
			fluidSim->velocityDissipation = velDiss;
		}

		fluidSim->update();
		lastBlock = pa->waveData.blockCounter;
	}
}

}
