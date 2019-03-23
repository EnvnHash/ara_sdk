//
// SNVideoTileAudio.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNVideoTileAudio.h"

namespace tav
{
SNVideoTileAudio::SNVideoTileAudio(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs)
{
	VideoTextureCv** vts = static_cast<VideoTextureCv**>(scd->videoTextures);
	vt = static_cast<VideoTextureCv*>(vts[0]);
	osc = static_cast<OSCData*>(scd->osc);
	pa = (PAudio*) scd->pa;
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	nrInstances = 4;

	texOffsMed = 25.f;
	texOffsPosMed = 10.f;
	pllDirCalcOffs = 0.1f;
	texOffsSpeed = 2.f;

	std::vector < coordType > instAttribs;
	instAttribs.push_back(MODMATR);
	instAttribs.push_back(TEXCORMOD);

	fNrInst = static_cast<float>(nrInstances);
	stepSize = 2.f / fNrInst;

	quad = new QuadArray(4, 4, -1.0f, -1.0f, 2.f, 2.f, 1.f, 1.f, 1.f, 1.f,
			&instAttribs, nrInstances * nrInstances);
	quad->rotate(M_PI, 1.f, 0.f, 0.f);

	nrTransInd = nrInstances * nrInstances;

	nrRandTable = 5;
	randTable = new float*[nrRandTable];
	for (auto i = 0; i < nrRandTable; i++)
	{
		randTable[i] = new float[nrTransInd];
		for (auto j = 0; j < nrTransInd; j++)
			randTable[i][j] = getRandF(0.f, 1.f);
	}

	texOffDir = new glm::vec2*[2];
	texOffPos = new glm::vec2*[2];
	for (auto i = 0; i < 2; i++)
	{
		texOffDir[i] = new glm::vec2[nrTransInd];
		texOffPos[i] = new glm::vec2[nrTransInd];

		for (auto j = 0; j < nrTransInd; j++)
		{
			texOffDir[i][j] = glm::vec2(0.f);
			texOffPos[i][j] = glm::vec2(0.f);
		}
	}

	transInd = new float[nrTransInd];

	for (auto i = 0; i < nrTransInd; i++)
	{
		transInd[i] = 0.f;
	}
}

//------------------------------------------------------------------------------------

void SNVideoTileAudio::draw(double time, double dt, camPar* cp,
		Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	sendStdShaderInit(_shader);
	_shader->setUniform1i("useInstancing", 1);

	vt->updateDt(dt * scd->sceneSpeed * osc->videoSpeed, static_cast<bool>(osc->startStopVideo));
	vt->loadFrameToTexture(time);

	useTextureUnitInd(0, vt->getTex(), _shader, _tfo);

	if (lastBlock != pa->getBlockCount())
	{
		updateModMatr(dt);
		lastBlock = pa->getBlockCount();
	}

	quad->drawInstanced(nrInstances * nrInstances, _tfo, 1.f);
}

//------------------------------------------------------------------------------------

void SNVideoTileAudio::update(double time, double dt)
{
}

//------------------------------------------------------------------------------------

void SNVideoTileAudio::updateModMatr(float dt)
{
	// Setup Matrices Map the buffer
	GLfloat* matrices = (GLfloat*) quad->getMapBuffer(MODMATR);

	for (auto y = 0; y < nrInstances; y++)
	{
		for (auto x = 0; x < nrInstances; x++)
		{
			int ind = y * nrInstances + x;

			// move up and down
			glm::mat4 scaleMatr = glm::scale(
					glm::vec3(1.f / fNrInst, 1.f / fNrInst, 1.f));
			glm::mat4 thisMatr = glm::translate(
					glm::vec3(
							static_cast<float>(x) * stepSize - 1.f
									+ (stepSize * 0.5f),
							static_cast<float>(y) * stepSize - 1.f
									+ (stepSize * 0.5f), 0.f));
			thisMatr = thisMatr * scaleMatr;

			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					matrices[ind * 16 + i * 4 + j] = thisMatr[i][j];
		}
	}

	quad->unMapBuffer();

	//------------------------------------

	// make the texcormods
	GLfloat* texCorMods = (GLfloat*) quad->getMapBuffer(TEXCORMOD);

	// pll Indexing offset for all instances
	transInd[1] = std::fmod(transInd[1] + dt * 0.1f, 1.f);

	for (auto y = 0; y < nrInstances; y++)
	{
		for (auto x = 0; x < nrInstances; x++)
		{
			int ind = y * nrInstances + x;
			float fInd = static_cast<float>(y * nrInstances + x)
					/ static_cast<float>(nrInstances * nrInstances);

			// nimm zwei samples aus dem pll und kalkuliere davon einen Richtungsvektor
			for (auto i = 0; i < 2; i++)
			{
				glm::vec2 dirAtSamp = glm::normalize(
						glm::vec2(((fInd + pllDirCalcOffs) - fInd) * 0.05f,
								pa->getPllSepBandAtPos(0, i,
										fInd + pllDirCalcOffs)
										- pa->getPllSepBandAtPos(0, i, fInd)));

				texOffDir[i][ind] = (dirAtSamp + texOffDir[i][ind] * texOffsMed)
						/ (texOffsMed + 1.f);
				texOffDir[i][ind] = glm::normalize(texOffDir[i][ind]);

				// addiere den normalisierten Richtungsvektor auf die aktuelle Offset Position drauf
				texOffPos[i][ind] = ((texOffPos[i][ind]
						+ texOffDir[i][ind] * texOffsSpeed * dt)
						+ texOffPos[i][ind] * texOffsPosMed)
						/ (1.f + texOffsPosMed);
			}

			glm::vec4 texMod = glm::vec4(texOffPos[0][ind].x * 2.f - 1.f, // offsx
			texOffPos[0][ind].y * 2.f - 1.f,   // offsy
					std::max(std::min(std::fabs(texOffPos[1][ind].x), 2.f),
							0.5f) * 0.4f,   // scale
					std::max(std::min(std::fabs(texOffPos[1][ind].y), 2.f),
							0.5f) * 0.4f);

			for (int i = 0; i < 4; i++)
				texCorMods[ind * 4 + i] = texMod[i];
		}
	}

	quad->unMapBuffer();
}

//------------------------------------------------------------------------------------

SNVideoTileAudio::~SNVideoTileAudio()
{
	delete quad;
}
}
