//
// SNAudioTunnel.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNAudioTunnel.h"

using namespace glm;

namespace tav
{
SNAudioTunnel::SNAudioTunnel(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "LitSphere")
{
	pa = (PAudio*) scd->pa;
	osc = (OSCData*) scd->osc;
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	getChanCols(&chanCols);

	nrXSegments = 80;
	nrYSegments = 20;
//        nrPllSnapShots = nrYSegments +1;
	nrPllSnapShots = nrYSegments - 1;
	nrPllSnapShotsC = std::max(nrPllSnapShots, 1);
//        nrPllSnapShotsC = std::max(nrPllSnapShots-1, 1);

	yStepSize = 1.f / static_cast<float>(nrYSegments);

	speed = 0.5f;

	ampScale = 0.4f;
	basicRadius = 5.f;
	ringHeight = 40.f;
	zOffs = 0.0f;
//        zOffs = ringHeight * 1.0f;
	rotPerChan = 0.125f;
	bright = 0.98f;

	flipNormals = -1.f;

	pllOffs = glm::vec3(0.1f, 0.2f, 0.3f);

	// CHANNELS DOWNMIX!!!!!
	mixDownToNrChans = 1;
	chanDownMix = new std::vector<short>[mixDownToNrChans];
	for (auto i = 0; i < scd->nrChannels; i++)
	{
		chanDownMix[i % mixDownToNrChans].push_back(i);
	}

	quadArrays = new QuadArray*[mixDownToNrChans];
	pllPositions = new glm::vec3**[mixDownToNrChans];
	pllNormals = new glm::vec3***[mixDownToNrChans];
	position = new glm::vec3**[mixDownToNrChans];
	pllTimeOffs = new float*[scd->nrChannels];
	modMatrPerChan = new glm::mat4[mixDownToNrChans];

	for (int i = 0; i < mixDownToNrChans; i++)
	{
		modMatrPerChan[i] = glm::translate(
				glm::vec3(0.f, -ringHeight - 2.f, 0.f))
				* glm::rotate(static_cast<float>(M_PI) * 0.f,
						glm::vec3(1.f, 0.f, 0.f));
//            modMatrPerChan[i] = glm::rotate(static_cast<float>(M_PI) * -0.5f, glm::vec3(1.f, 0.f, 0.f));

		glm::mat4 rotM = glm::rotate(getRandF(0.f, 1.f) * rotPerChan,
				normalize(
						glm::vec3(getRandF(0.f, 1.f), getRandF(0.f, 1.f),
								getRandF(0.f, 1.f))));

		modMatrPerChan[i] = rotM * modMatrPerChan[i];
		modMatrPerChan[i][3][2] = zOffs;

		int colInd = i % MAX_NUM_COL_SCENE;

		quadArrays[i] = new QuadArray(nrXSegments, nrYSegments, -1.f, -1.f, 2.f,
				2.f, std::fmin(chanCols[colInd].r * bright, 1.f),
				std::fmin(chanCols[colInd].g * bright, 1.f),
				std::fmin(chanCols[colInd].b * bright, 1.f),
				std::fmin(chanCols[colInd].a * bright, 1.f));

		position[i] = new glm::vec3*[nrYSegments];
		pllNormals[i] = new glm::vec3**[nrYSegments];

		for (int y = 0; y < nrYSegments; y++)
		{
			position[i][y] = new glm::vec3[nrXSegments * 6];
			pllNormals[i][y] = new glm::vec3*[4];
			for (auto j = 0; j < 4; j++)
				pllNormals[i][y][j] = new glm::vec3[nrXSegments];
		}

		pllPositions[i] = new glm::vec3*[nrPllSnapShots];

		for (int y = 0; y < nrPllSnapShots; y++)
		{
			pllPositions[i][y] = new glm::vec3[nrXSegments];
			for (auto x = 0; x < nrPllSnapShots; x++)
				pllPositions[i][y][x] = glm::vec3(0.f, 0.f, 0.f);
		}
	}

	for (int i = 0; i < scd->nrChannels; i++)
	{
		pllTimeOffs[i] = new float[nrPllSnapShots];
		for (int y = 0; y < nrPllSnapShots; y++)
		{
			pllTimeOffs[i][y] = 1.f;
		}
	}

	lastTime = -1.f;
}

//----------------------------------------------------

SNAudioTunnel::~SNAudioTunnel()
{
	for (int i = 0; i < scd->nrChannels; i++)
	{
		delete pllTimeOffs[i];
	}

	for (int i = 0; i < mixDownToNrChans; i++)
	{
		delete quadArrays[i];

		for (int y = 0; y < nrPllSnapShots; y++)
		{
			delete pllPositions[i][y];
		}

		for (int y = 0; y < nrYSegments; y++)
		{
			delete position[i][y];
			delete pllNormals[i][y];
		}

		delete pllPositions[i];
	}
	delete quadArrays;
}

//----------------------------------------------------

void SNAudioTunnel::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);
	}

	sendStdShaderInit(_shader);

	for (auto i = 0; i < mixDownToNrChans; i++)
		quadArrays[i]->draw(_tfo);
}

//----------------------------------------------------

void SNAudioTunnel::update(double time, double dt)
{
	if (lastBlock != pa->waveData.blockCounter)
	{
		for (int i = 0; i < mixDownToNrChans; i++)
		{
			for (int y = 0; y < nrPllSnapShots; y++)
			{
				float fInd = static_cast<float>(y)
						/ static_cast<float>(nrPllSnapShots);
				float zPos = fmod(fInd + (time * speed), 1.0f);

				if (pllTimeOffs[i][y] > 0.5f && zPos < 0.5f)
					updateWave(i, y);

				pllTimeOffs[i][y] = zPos;
			}
		}

		updateGeo();
		updateVAO(time);

		lastBlock = pa->waveData.blockCounter;
	}
}

//----------------------------------------------------
// take a pll snapshot
void SNAudioTunnel::updateWave(int mixDownChanInd, int pllInd)
{
	for (int n = 0; n < nrXSegments; n++)
	{
		float val[3] =
		{ 0.f, 0.f, 0.f };

		for (short j = 0; j < (short) chanDownMix[mixDownChanInd].size(); j++)
		{
			float fInd = static_cast<float>(n)
					/ static_cast<float>(nrXSegments - 1);

			val[0] += pa->getPllAtPos(
					scd->chanMap[chanDownMix[mixDownChanInd][j]],
					std::fmod(fInd + pllOffs.x, 1.f));
			val[1] += pa->getPllAtPos(
					scd->chanMap[chanDownMix[mixDownChanInd][j]],
					std::fmod(fInd + pllOffs.y, 1.f));
			val[2] += pa->getPllAtPos(
					scd->chanMap[chanDownMix[mixDownChanInd][j]],
					std::fmod(fInd + pllOffs.z, 1.f));
		}

		for (auto i = 0; i < 3; i += 2)
			val[i] = (val[i] * ampScale + 1.f) * 0.5f;

		pllPositions[mixDownChanInd][pllInd][n] = glm::vec3(val[0], val[1],
				val[2]);
	}
}

//----------------------------------------------------

void SNAudioTunnel::updateGeo()
{
	// first pass, positions and lower edge normals
	for (auto cNr = 0; cNr < mixDownToNrChans; cNr++)
	{
		// tunnel wird von "unten": yPos=0 nach "oben" yPos = 1 gezeichnet
		for (auto y = 0; y < nrYSegments; y++)
		{
			float toInd = (1.0f - pllTimeOffs[cNr][y])
					* static_cast<float>(nrPllSnapShotsC);
			int lowerInd = static_cast<int>(floor(toInd));
			lowerInd = (lowerInd - 1 + nrPllSnapShots) % nrPllSnapShots;
			int upperInd = (lowerInd + 1) % nrPllSnapShots;
			float weight = toInd - floor(toInd);

			for (auto nInd = 0; nInd < nrXSegments + 1; nInd++)
			{
				int n = nInd % nrXSegments;
				for (int q = 0; q < 6; q++)
				{
					float fInd = 0.f;
					float yPos = y * yStepSize;
					glm::vec3 excent = glm::vec3(0.f);

					int pllInd = (q > 1 && q < 5) ? n + 1 : n;
					pllInd %= nrXSegments;
					fInd = static_cast<float>(pllInd)
							/ static_cast<float>(nrXSegments); // quad rechte seite

					if (q > 0 && q < 4)
					{
						// quad untere seite
						excent = pllPositions[cNr][lowerInd][pllInd]
								* (1.0f - weight)
								+ pllPositions[cNr][upperInd][pllInd] * weight;

						if (y == 0)
						{
							position[cNr][y][n * 6 + q].x = std::cos(
									fInd * TWO_PI) * basicRadius * excent.x;
							position[cNr][y][n * 6 + q].y = ringHeight * yPos;
							position[cNr][y][n * 6 + q].z = std::sin(
									fInd * TWO_PI) * basicRadius * excent.x;
						}
						else
						{
							// wenn das erste segment berechnet wurde
							// nimm als untere seite, die obere seite des vorigen segments
							position[cNr][y][n * 6 + q] = position[cNr][y - 1][n
									* 6 + std::min((q - 1) * 4, 4)];
						}
					}
					else
					{
						int lowerIndPlusOne = (lowerInd + 1) % nrPllSnapShotsC;
						int upperIndPlusOne = (upperInd + 1) % nrPllSnapShotsC;

						// quad obere seite
						yPos = (y + 1) * yStepSize;

						excent = pllPositions[cNr][lowerIndPlusOne][pllInd]
								* (1.0f - weight)
								+ pllPositions[cNr][upperIndPlusOne][pllInd]
										* weight;

						position[cNr][y][n * 6 + q].x = std::cos(fInd * TWO_PI)
								* basicRadius * excent.x;
						position[cNr][y][n * 6 + q].y = ringHeight * yPos;
						position[cNr][y][n * 6 + q].z = std::sin(fInd * TWO_PI)
								* basicRadius * excent.x;
					}
				}

				// berechne das kreuzprodukt an der linken unteren ecke des quads
				glm::vec3 p1p0 = position[cNr][y][n * 6 + 1]
						- position[cNr][y][n * 6];
				glm::vec3 p1p2 = position[cNr][y][n * 6 + 1]
						- position[cNr][y][n * 6 + 2];
				pllNormals[cNr][y][0][n] = cross(p1p0, p1p2);

				// berechne das kreuzprodukt an der rechten unteren ecke des quads
				glm::vec3 p2p1 = position[cNr][y][n * 6 + 2]
						- position[cNr][y][n * 6 + 1];
				glm::vec3 p2p4 = position[cNr][y][n * 6 + 2]
						- position[cNr][y][n * 6 + 4];
				pllNormals[cNr][y][1][n] = cross(p2p1, p2p4);

				// bei mehreren Y-Segmenten könnte dieser schritt ab der zweiten
				// reihe übersprungen werden... ergebnis wäre nur unwesentlich anders

				// berechne das kreuzprodukt an der rechten oberen ecke des quads
				glm::vec3 p4p0 = position[cNr][y][n * 6 + 4]
						- position[cNr][y][n * 6];
				glm::vec3 p4p3 = position[cNr][y][n * 6 + 4]
						- position[cNr][y][n * 6 + 3];
				// wird in die falsche richtung zeigen, deshalb * -1;
				pllNormals[cNr][y][2][n] = cross(p4p0, p4p3) * -1.f;

				// berechne das kreuzprodukt an der linken oberen ecke des quads
				glm::vec3 p0p4 = position[cNr][y][n * 6]
						- position[cNr][y][n * 6 + 4];
				glm::vec3 p0p1 = position[cNr][y][n * 6]
						- position[cNr][y][n * 6 + 1];
				pllNormals[cNr][y][3][n] = cross(p0p4, p0p1);

				//                for (int k=0;k<4;k++)
				//                    std::cout << "normal [" << k << "]: " <<  glm::to_string(pllNormals[cNr][k][n]) << std::endl;
				//                std::cout << std::endl;

				// wenn n grösser als 1 bilde für die linke untere ecke des quads
				// einen mittelwert mit der rechten unteren Kante des vorigen Segments
				// später werden für die linke untere und rechte untere ecke nur
				// dieser wert genommen (index:0)
				if (nInd > 0)
				{
					pllNormals[cNr][y][0][n] = normalize(
							(pllNormals[cNr][y][0][n]
									+ pllNormals[cNr][y][1][n - 1]) * 0.5f)
							* flipNormals;

					// für die erste reihe der y-segmente auch die oberen ecken interpolieren
					pllNormals[cNr][y][3][n] = normalize(
							(pllNormals[cNr][y][3][n]
									+ pllNormals[cNr][y][2][n - 1]) * 0.5f)
							* flipNormals;

					if (y > 0)
					{
						pllNormals[cNr][y][0][n] = normalize(
								(pllNormals[cNr][y][0][n]
										+ pllNormals[cNr][y - 1][3][n]) * -0.5f)
								* flipNormals;
						pllNormals[cNr][y - 1][3][n] = pllNormals[cNr][y][0][n];
						pllNormals[cNr][y][3][n] = normalize(
								(pllNormals[cNr][y][3][n]
										+ pllNormals[cNr][y - 1][1][n]) * -0.5f)
								* flipNormals;
					}
				}
			}
		}
	}
}

//----------------------------------------------------

void SNAudioTunnel::updateVAO(double _time)
{
	glm::mat4 riseMat = glm::rotate(float(osc->zoom * M_PI * 0.5f),
			glm::vec3(1.f, 0.f, 0.f))
			* glm::translate(glm::vec3(0.f, 3.f, 0.f));

	for (auto cNr = 0; cNr < mixDownToNrChans; cNr++)
	{
		// update modMatr with continous rotation
		glm::mat4 rotMat = modMatrPerChan[cNr];

		// update positions
		GLfloat* pos = (GLfloat*) quadArrays[cNr]->getMapBuffer(POSITION);

		for (auto y = 0; y < nrYSegments; y++)
		{
			for (auto n = 0; n < nrXSegments; n++)
			{
				for (auto q = 0; q < 6; q++)
				{
					glm::vec4 modvec = riseMat * rotMat
							* glm::vec4(position[cNr][y][n * 6 + q], 1.f);
					pos[((y * nrXSegments + n) * 6 + q) * 3] = modvec.x;
					pos[((y * nrXSegments + n) * 6 + q) * 3 + 1] = modvec.y;
					pos[((y * nrXSegments + n) * 6 + q) * 3 + 2] = modvec.z;
				}
			}
		}

		quadArrays[cNr]->unMapBuffer();

		GLfloat* nor = (GLfloat*) quadArrays[cNr]->getMapBuffer(NORMAL);

		for (auto y = 0; y < nrYSegments; y++)
		{
			for (int n = 1; n < nrXSegments; n++)
			{
				glm::vec3 modNor1 = glm::mat3(rotMat)
						* pllNormals[cNr][y][3][n];
				glm::vec3 modNor2 = glm::mat3(rotMat)
						* pllNormals[cNr][y][0][n];

				// links oben das vorigen quads
				nor[((y * nrXSegments + n) * 6) * 3] = modNor1.x;
				nor[((y * nrXSegments + n) * 6) * 3 + 1] = modNor1.y;
				nor[((y * nrXSegments + n) * 6) * 3 + 2] = modNor1.z;

				// links oben das vorigen quads
				nor[((y * nrXSegments + n) * 6 + 5) * 3] = modNor1.x;
				nor[((y * nrXSegments + n) * 6 + 5) * 3 + 1] = modNor1.y;
				nor[((y * nrXSegments + n) * 6 + 5) * 3 + 2] = modNor1.z;

				//  setze die rechte obere des vorigen mit
				// rechts oben das aktuellen quads
				nor[((y * nrXSegments + n - 1) * 6 + 4) * 3] = modNor1.x;
				nor[((y * nrXSegments + n - 1) * 6 + 4) * 3 + 1] = modNor1.y;
				nor[((y * nrXSegments + n - 1) * 6 + 4) * 3 + 2] = modNor1.z;

				// links unten das vorigen quads
				nor[((y * nrXSegments + n) * 6 + 1) * 3] = modNor2.x;
				nor[((y * nrXSegments + n) * 6 + 1) * 3 + 1] = modNor2.y;
				nor[((y * nrXSegments + n) * 6 + 1) * 3 + 2] = modNor2.z;

				//  setze die rechte untere des vorigen mit
				// rechts unten das aktuellen quads
				nor[((y * nrXSegments + n - 1) * 6 + 2) * 3] = modNor2.x;
				nor[((y * nrXSegments + n - 1) * 6 + 2) * 3 + 1] = modNor2.y;
				nor[((y * nrXSegments + n - 1) * 6 + 2) * 3 + 2] = modNor2.z;

				// rechts unten das aktuellen quads
				nor[((y * nrXSegments + n - 1) * 6 + 3) * 3] = modNor2.x;
				nor[((y * nrXSegments + n - 1) * 6 + 3) * 3 + 1] = modNor2.y;
				nor[((y * nrXSegments + n - 1) * 6 + 3) * 3 + 2] = modNor2.z;
			}
		}

		quadArrays[cNr]->unMapBuffer();

		GLfloat* tex = (GLfloat*) quadArrays[cNr]->getMapBuffer(TEXCOORD);

		for (auto y = 0; y < nrYSegments; y++)
		{
			for (int n = 0; n < nrXSegments; n++)
			{
				for (auto q = 0; q < 6; q++)
				{
					if (q == 0 || q == 4 || q == 5)
						tex[((y * nrXSegments + n) * 6 + q) * 2 + 1] =
								std::fmod(
										float(y + 1) / float(nrYSegments)
												+ (_time * speed), 1.f);
					else
						tex[((y * nrXSegments + n) * 6 + q) * 2 + 1] =
								std::fmod(
										float(y) / float(nrYSegments)
												+ (_time * speed), 1.f);

					if (q == 0 || q == 1 || q == 5)
						tex[((y * nrXSegments + n) * 6 + q) * 2] = float(n)
								/ float(nrXSegments);
					else
						tex[((y * nrXSegments + n) * 6 + q) * 2] = float(n + 1)
								/ float(nrXSegments);
				}
			}
		}

		quadArrays[cNr]->unMapBuffer();
	}
}

}
