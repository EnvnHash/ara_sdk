//
// SNParaColumns.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNParaColumns.h"

namespace tav
{
SNParaColumns::SNParaColumns(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs), nrCubes(300)
{
	pa = (PAudio*) scd->pa;
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	getChanCols(&chanCols);

	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];

	cubesPerChan = nrCubes / scd->nrChannels;

	depthScale = -15.0f;
	yOffs = -1.5f;
	speed = 0.5f;
	sizeScale = 10.0f; // für die horizontale Skalierung im Bezug auf die Lautstärke

	// räumliches offsets pro kanal
	chanSpaceMap = new glm::vec3[4];
	chanSpaceMap[0] = glm::vec3(1.5f, 0.f, 0.f);
	chanSpaceMap[1] = glm::vec3(-1.5f, 0.f, 0.f);
	chanSpaceMap[2] = glm::vec3(0.f, 1.5f, 0.f);
	chanSpaceMap[3] = glm::vec3(0.f, -1.5f, 0.f);

	cubePars = new cubePar[nrCubes];
	for (int n = 0; n < nrCubes; n++)
	{
		cubePars[n].zPos = 1.0f;
		cubePars[n].on = 0.0f;
		cubePars[n].amp = 0.0f;
	}

	// prepare for instancing, MOD_MATR will scd->chanMap[chan]ge every instance
	std::vector < coordType > instAttribs;
	instAttribs.push_back(TEXCORMOD);
	instAttribs.push_back(MODMATR);

	cubes = new Cube(1.f, 1.f, 1.f, 1, &instAttribs, nrCubes, 0.1f, 0.1f, 0.1f,
			1.0f);
	//cubes = new Cube(1.f, 1.f, 1.f, &instAttribs, nrCubes, 0.1f, 0.1f, 0.1f, 1.0f);
	//cubes->scale(0.1f, 0.1f, 0.1f);
}

//----------------------------------------------------

SNParaColumns::~SNParaColumns()
{
	delete[] cubePars;
	delete cubes;
	delete[] chanSpaceMap;
}

//----------------------------------------------------

void SNParaColumns::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	sendStdShaderInit(_shader);

	if (_tfo)
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (lastBlock != pa->waveData.blockCounter)
	{
		// Setup Matrices Map the buffer
		GLfloat* matrices = (GLfloat*) cubes->getMapBuffer(MODMATR);

		// Set model matrices for each instance
		for (auto chan = 0; chan < scd->nrChannels; chan++)
		{
			for (auto n = 0; n < cubesPerChan; n++)
			{
				int cubeInd = scd->chanMap[chan] * cubesPerChan + n;
				float fInd = static_cast<float>(n)
						/ static_cast<float>(cubesPerChan);
				float cubeZPos = fmod(fInd + (time * speed), 1.0f) * depthScale
						- 5.f;

				// wenn über den "rand" gelaufen
				if (cubePars[cubeInd].zPos / depthScale > 0.5f
						&& cubeZPos / depthScale < 0.5f)
				{
					cubePars[cubeInd].on =
							pa->getCatchedOnset(scd->chanMap[chan]) ?
									1.0f : 0.0f;
					cubePars[cubeInd].amp = pa->getCatchedAmp(
							scd->chanMap[chan]);
					cubePars[cubeInd].height = std::sqrt(
							pa->getCatchedAmp(scd->chanMap[chan])) * 25.0f
							* cubePars[cubeInd].on;
					cubePars[cubeInd].width = std::pow(
							1.f
									- std::fmin(
											pa->getPitch(scd->chanMap[chan])
													/ 6000.f, 1.f), 2.f) * 0.4f;
					cubePars[cubeInd].depth = pa->getSepBandEnergy(
							scd->chanMap[chan], 2) * 0.1f;
//                        cubePars[cubeInd].xOffs = 0.f;
					cubePars[cubeInd].xOffs = pa->getPllAtPos(
							scd->chanMap[chan], 0.1f)
							* chanSpaceMap[scd->chanMap[chan] % 4].x * 2.f;
					cubePars[cubeInd].texOffsX = pa->getPllAtPos(
							scd->chanMap[chan], 0.2f)
							* chanSpaceMap[scd->chanMap[chan] % 4].x;
					cubePars[cubeInd].texOffsY = pa->getPllAtPos(
							scd->chanMap[chan], 0.3f)
							* chanSpaceMap[scd->chanMap[chan] % 4].y;
				}
				cubePars[cubeInd].zPos = cubeZPos;

				glm::mat4 thisMatr = glm::translate(
						glm::vec3(
								chanSpaceMap[scd->chanMap[chan] % maxNrChans].x
										+ cubePars[cubeInd].xOffs,
								chanSpaceMap[scd->chanMap[chan] % maxNrChans].y
										+ cubePars[cubeInd].height * 0.5f
										+ yOffs, depthScale - cubeZPos));

				glm::mat4 scaleMatr = glm::scale(
						glm::vec3(
								cubePars[cubeInd].on * cubePars[cubeInd].width
										* 2.f, cubePars[cubeInd].height,
								cubePars[cubeInd].on
										* cubePars[cubeInd].depth));
				thisMatr = thisMatr * scaleMatr;

				for (int i = 0; i < 4; i++)
					for (int j = 0; j < 4; j++)
						matrices[cubeInd * 16 + i * 4 + j] = thisMatr[i][j];
			}
		}
		cubes->unMapBuffer();

		// make the texcormods
		GLfloat* texCorMods = (GLfloat*) cubes->getMapBuffer(TEXCORMOD);
		for (auto chan = 0; chan < scd->nrChannels; chan++)
		{
			for (auto n = 0; n < cubesPerChan; n++)
			{
				int cubeInd = scd->chanMap[chan] * cubesPerChan + n;
				glm::vec4 texMod = glm::vec4(cubePars[cubeInd].texOffsX,
						cubePars[cubeInd].texOffsY,
						cubePars[cubeInd].height * 0.1f,
						cubePars[cubeInd].width);

				for (int i = 0; i < 4; i++)
					texCorMods[cubeInd * 4 + i] = texMod[i];
			}
		}

		cubes->unMapBuffer();

		lastBlock = pa->waveData.blockCounter;
	}

	useTextureUnitInd(0, tex0->getId(), _shader, _tfo);

	_shader->setUniform1i("useInstancing", nrCubes);
	cubes->drawInstanced(nrCubes, _tfo, 1.f);
}

//----------------------------------------------------

void SNParaColumns::update(double time, double dt)
{
}

//----------------------------------------------------

void SNParaColumns::onKey(int key, int scancode, int action, int mods)
{
}
}
