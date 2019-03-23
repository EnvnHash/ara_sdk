//
// SNVideoTileAudio3d.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNVideoTileAudio3d.h"

namespace tav
{
SNVideoTileAudio3d::SNVideoTileAudio3d(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	VideoTextureCv** vts = static_cast<VideoTextureCv**>(scd->videoTextures);
	vt = static_cast<VideoTextureCv*>(vts[0]);
	osc = static_cast<OSCData*>(scd->osc);
	pa = (PAudio*) scd->pa;
	getChanCols(&chanCols);

	nrInstX = 4;
	nrInstY = 4;
	nrInstZ = 8;

	texOffsMed = 25.f;
	texOffsPosMed = 10.f;
	pllDirCalcOffs = 0.1f;
	texOffsSpeed = 3.f;

	std::vector < coordType > instAttribs;
	instAttribs.push_back(MODMATR);
	instAttribs.push_back(TEXCORMOD);

	fNrInst = glm::vec3(static_cast<float>(nrInstX),
			static_cast<float>(nrInstY), static_cast<float>(nrInstZ));

	stepSize = glm::vec3(2.f / fNrInst.x, 2.f / fNrInst.y, 1.f / fNrInst.z);

	quad = new QuadArray(4, 4, -1.0f, -1.0f, 2.f, 2.f, 0.f, 0.f, 0.f, 1.f,
			&instAttribs, nrInstX * nrInstY * nrInstZ);
	quad->rotate(M_PI, 1.f, 0.f, 0.f);

	nrTransInd = nrInstX * nrInstY * nrInstZ;

	nrRandTable = 5;
	randTable = new float*[nrRandTable];
	for (auto i = 0; i < nrRandTable; i++)
	{
		randTable[i] = new float[nrTransInd];
		for (auto j = 0; j < nrTransInd; j++)
			randTable[i][j] = getRandF(0.f, 1.f);
	}

	nrTexOffPos = 3;
	texOffDir = new glm::vec2*[nrTexOffPos];
	texOffPos = new glm::vec2*[nrTexOffPos];
	for (auto i = 0; i < nrTexOffPos; i++)
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

//------------------------------------------

void SNVideoTileAudio3d::draw(double time, double dt, camPar* cp,
		Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setSceneNodeColors(chanCols);
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	sendStdShaderInit(_shader);
	_shader->setUniform1i("useInstancing", 1);

	vt->updateDt(dt * scd->sceneSpeed * osc->videoSpeed,
			static_cast<bool>(osc->startStopVideo));
	vt->loadFrameToTexture(time);

	useTextureUnitInd(0, vt->getTex(), _shader, _tfo);

	if (lastBlock != pa->getBlockCount())
	{
		updateModMatr(dt);
		lastBlock = pa->getBlockCount();
	}

	quad->drawInstanced(nrInstX * nrInstY * nrInstZ, _tfo, 1.f);
}

//------------------------------------------

void SNVideoTileAudio3d::update(double time, double dt)
{
}

//------------------------------------------

void SNVideoTileAudio3d::updateModMatr(float dt)
{
	// Setup Matrices Map the buffer
	GLfloat* matrices = (GLfloat*) quad->getMapBuffer(MODMATR);

	for (auto z = 0; z < nrInstZ; z++)
	{
		for (auto y = 0; y < nrInstY; y++)
		{
			for (auto x = 0; x < nrInstX; x++)
			{
				int ind = (z * nrInstY * nrInstX) + (y * nrInstX) + x;
				float fInd = static_cast<float>(ind)
						/ static_cast<float>(nrInstZ * nrInstY * nrInstX);

				transInd[ind] = std::fmod(transInd[ind] + (dt * 0.2f), 1.f);

				glm::vec2 dirAtSamp = glm::normalize(
						glm::vec2(((fInd + pllDirCalcOffs) - fInd) * 0.05f,
								pa->getPllSepBandAtPos(0, 2,
										transInd[ind] + fInd + pllDirCalcOffs)
										- pa->getPllSepBandAtPos(0, 2,
												transInd[ind] + fInd)));

				texOffDir[2][ind] = (dirAtSamp + texOffDir[2][ind] * texOffsMed)
						/ (texOffsMed + 1.f);
				texOffDir[2][ind] = glm::normalize(texOffDir[2][ind]);

				// addiere den normalisierten Richtungsvektor auf die aktuelle Offset Position drauf
				texOffPos[2][ind] = ((texOffPos[2][ind]
						+ texOffDir[2][ind] * texOffsSpeed * dt * 5.f)
						+ texOffPos[2][ind] * 4.f) / (5.f);

				texOffPos[2][ind].x = std::min(
						std::max(texOffPos[2][ind].x, -1.f), 1.f);
				texOffPos[2][ind].y = std::min(
						std::max(texOffPos[2][ind].y, -1.f), 1.f);

				// move up and down
				glm::mat4 scaleMatr = glm::scale(
						glm::vec3(
								1.f / fNrInst.x
										* ((texOffPos[2][ind].y * 0.7f + 1.5f)),
								1.f / fNrInst.y
										* ((texOffPos[2][ind].y * 0.7f + 1.5f)),
								1.f));
				glm::mat4 thisMatr = glm::translate(
						glm::vec3(
								static_cast<float>(x) * stepSize.x - 1.f
										+ (stepSize.x * 0.5f),
								static_cast<float>(y) * stepSize.y - 1.f
										+ (stepSize.y * 0.5f),
								static_cast<float>(z) * stepSize.z * -0.75f
										+ texOffPos[2][ind].y * 0.25f));

				thisMatr = thisMatr * scaleMatr;

				for (int i = 0; i < 4; i++)
					for (int j = 0; j < 4; j++)
						matrices[ind * 16 + i * 4 + j] = thisMatr[i][j];
			}
		}
	}

	quad->unMapBuffer();

	// make the texcormods
	GLfloat* texCorMods = (GLfloat*) quad->getMapBuffer(TEXCORMOD);

	// pll Indexing offset for all instances
	transInd[1] = std::fmod(transInd[1] + dt * 0.1f, 1.f);

	for (auto z = 0; z < nrInstZ; z++)
	{
		for (auto y = 0; y < nrInstY; y++)
		{
			for (auto x = 0; x < nrInstX; x++)
			{
				int ind = (z * nrInstY * nrInstX) + (y * nrInstX) + x;
				float fInd = static_cast<float>(ind)
						/ static_cast<float>(nrInstZ * nrInstY * nrInstX);

				// nimm zwei samples aus dem pll und kalkuliere davon einen Richtungsvektor
				for (auto i = 0; i < 2; i++)
				{
					glm::vec2 dirAtSamp = glm::normalize(
							glm::vec2(((fInd + pllDirCalcOffs) - fInd) * 0.05f,
									pa->getPllSepBandAtPos(0, i,
											fInd + pllDirCalcOffs)
											- pa->getPllSepBandAtPos(0, i,
													fInd)));

					texOffDir[i][ind] = (dirAtSamp
							+ texOffDir[i][ind] * texOffsMed)
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
	}

	quad->unMapBuffer();
}

//------------------------------------------

SNVideoTileAudio3d::~SNVideoTileAudio3d()
{
	delete quad;
}
}
