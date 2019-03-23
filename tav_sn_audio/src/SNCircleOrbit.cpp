//
// SNCircleOrbit.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNCircleOrbit.h"

using namespace glm;

namespace tav
{
SNCircleOrbit::SNCircleOrbit(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "LitSphere")
{
	pa = (PAudio*) scd->pa;
	osc = (OSCData*) scd->osc;
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	getChanCols(&chanCols);

	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];

	nrPartCircs = 15;
	nrInst = 28;

	yAmp = 3.f;
	circBaseSize = 1.5f;
	rotSpeed = 0.07f;
	depthScale = 20.0f;
	scaleAmt = 4.f;

	std::vector < coordType > instAttribs;
	instAttribs.push_back(MODMATR);
	// instAttribs.push_back(TEXCORMOD);

	// mach 8 verschiedene Circles mit unterschiedlichen Stärken
	circs = new Circle*[nrPartCircs];
	cp = new circPar*[nrPartCircs];

	for (auto i = 0; i < nrPartCircs; i++)
	{
		float fInd = static_cast<float>(i)
				/ static_cast<float>(std::max(nrPartCircs - 1, 1));

		int colInd = i % MAX_NUM_COL_SCENE;
		// für jeden Circle mach Instanzes für jeden Kanal
		circs[i] = new Circle(30, 1.0f, 1.0f - ((fInd + 0.2f) * 0.12f),
				PI * getRandF(0.1, 0.25f), std::fmin(chanCols[colInd].r, 1.f),
				std::fmin(chanCols[colInd].g, 1.f),
				std::fmin(chanCols[colInd].b, 1.f),
				std::fmin(chanCols[colInd].a, 1.f), &instAttribs,
				nrInst * scd->nrChannels);
		circs[i]->rotate(M_PI * 2.f * getRandF(0.1, 1.f), 0.f, 0.f, 1.f);

		cp[i] = new circPar[nrInst * scd->nrChannels];

		for (auto j = 0; j < nrInst * scd->nrChannels; j++)
		{
			cp[i][j].pos = glm::vec3(0.f, 0.f, 0.f);
			cp[i][j].rotZ = getRandF(0.01f, 1.f);
		}
	}

	sLoop = new SpaceLoop*[scd->nrChannels];
	for (auto i = 0; i < scd->nrChannels; i++)
		sLoop[i] = new SpaceLoop(nrInst, depthScale, depthScale * -0.5f, 0.1f);

	partOffs = 1.f / static_cast<float>(nrPartCircs)
			* static_cast<float>(depthScale);
	addPar("alpha", &alpha);
}

//----------------------------------------------------

SNCircleOrbit::~SNCircleOrbit()
{
	delete circs;
	delete cp;
	delete[] sLoop;
}

//----------------------------------------------------

void SNCircleOrbit::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);
	}

	glClear (GL_DEPTH_BUFFER_BIT);
	sendStdShaderInit(_shader);

	if (lastBlock != pa->waveData.blockCounter)
	{
		updateModMatr(dt * osc->speed);
		lastBlock = pa->waveData.blockCounter;
	}

	_shader->setUniform1i("useInstancing", 1);
	useTextureUnitInd(0, scd->tex[0], _shader, _tfo);
	_shader->setUniform1f("oscAlpha", alpha * osc->alpha);
	_shader->setUniformMatrix4fv("modelMatrix", &_modelMat[0][0]);

	for (auto i = 0; i < scd->nrChannels; i++)
	{
		sLoop[i]->setSpeed(osc->speed * 0.25f);
		sLoop[i]->update(dt);
	}

	for (auto i = 0; i < nrPartCircs; i++)
		circs[i]->drawInstanced(nrInst * scd->nrChannels, _tfo, 1.f);
}

//----------------------------------------------------

void SNCircleOrbit::update(double time, double dt)
{
}

//----------------------------------------------------

void SNCircleOrbit::updateModMatr(float dt)
{
	for (auto cNr = 0; cNr < nrPartCircs; cNr++)
	{
		float nrPartInd = static_cast<float>(cNr)
				/ static_cast<float>(std::max(nrPartCircs - 1, 1));

		// Setup Matrices Map the buffer
		GLfloat* matrices = (GLfloat*) circs[cNr]->getMapBuffer(MODMATR);

		// Set model matrices for each instance
		for (auto chan = 0; chan < scd->nrChannels; chan++)
		{
			float chanInd = static_cast<float>(chan)
					/ static_cast<float>(std::max(scd->nrChannels - 1, 1));

			for (auto instNr = 0; instNr < nrInst; instNr++)
			{
				int instInd = chan * nrInst + instNr;
				float fInstInd = static_cast<float>(instNr)
						/ static_cast<float>(std::max(nrInst - 1, 1));
				float fInstInd2 = static_cast<float>(instNr)
						/ static_cast<float>(std::max(nrInst, 1));

				cp[cNr][instInd].pos.x = (chanInd - 0.5f) * 0.4f;
				cp[cNr][instInd].pos.y = (fInstInd - 0.5f)
						* pa->getMedAmp(scd->chanMap[chan]) * yAmp
						+ (nrPartInd * 0.2f);
				cp[cNr][instInd].pos.z = sLoop[chan]->getPos(instNr)
						+ nrPartInd * partOffs;
				cp[cNr][instInd].rotZ += (pa->getPllAtPos(scd->chanMap[chan],
						fmod(fInstInd + nrPartInd, 1.f)) + dt * 0.1f)
						* rotSpeed;
				if (cp[cNr][instInd].rotZ > 1.f)
					cp[cNr][instInd].rotZ = 0.f;

				// move up and down
				glm::mat4 thisMatr = glm::translate(cp[cNr][instInd].pos);

				// turn around z axis
				glm::mat4 rotMatr = glm::rotate(
						cp[cNr][instInd].rotZ * static_cast<float>(M_PI) * 2.f,
						glm::vec3(0.0f, 0.f, 1.f));

				// kipp es ein wenig zur seite
				glm::mat4 rotMatr2 = glm::rotate(
						(chanInd
								+ pa->getSmoothSpectrAt(scd->chanMap[chan],
										nrPartInd) * 16.f),
						normalize(glm::vec3(chanInd * 0.1f - 0.05f, 0.f, 1.f)));
				// skalier es
				float scalX = pa->getPllAtPos(scd->chanMap[chan],
						fmod(fInstInd2 + dt * 0.1f, 1.f)) * scaleAmt
						+ circBaseSize;
				float scalY = pa->getPllAtPos(scd->chanMap[chan],
						fmod(fInstInd2 + dt * 0.1f + 0.2f, 1.f)) * scaleAmt
						+ circBaseSize;

				glm::mat4 scaleMatr = glm::scale(glm::vec3(scalX, scalY, 1.f));
				thisMatr = thisMatr * rotMatr * rotMatr2 * scaleMatr
						* _modelMat;

				for (auto i = 0; i < 4; i++)
					for (auto j = 0; j < 4; j++)
						matrices[instInd * 16 + i * 4 + j] = thisMatr[i][j];
			}
		}

		circs[cNr]->unMapBuffer();

		/*
		 // make the texcormods
		 GLfloat* texCorMods = (GLfloat*) circs[cNr]->getMapBuffer(TEXCORMOD);
		 for (auto chan=0;chan<scd->nrChannels;chan++)
		 {
		 for (auto instNr=0;instNr<nrInst;instNr++)
		 {
		 int instInd = chan * nrInst + instNr;
		 glm::vec4 texMod = glm::vec4(cp[instInd].texOffsX, cp[instInd].texOffsY,
		 cp[instInd].height * 0.1f, cp[instInd].width);
		 
		 for (int i=0;i<4;i++)
		 texCorMods[instNr*4+i] = texMod[i];
		 }
		 }
		 
		 circs[cNr]->unMapBuffer();
		 */
	}
}

}
