//
// SNAudioWaveStrip.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNAudioWaveStrip.h"

using namespace glm;

namespace tav
{

SNAudioWaveStrip::SNAudioWaveStrip(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "LitSphere")
{
	pa = (PAudio*) scd->pa;
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	getChanCols(&chanCols);

	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];

	nrLines = 300;
	ampScale = 1.5f;
	lineWidth = 0.025f;

	nrPar = 3;
	signals = new float[nrPar];
	for (auto i = 0; i < nrPar; i++)
		signals[i] = 0.f;

	pllOffs = new float[nrPar];
	pllOffs[0] = 0.1f;
	pllOffs[1] = 0.2f;
	pllOffs[2] = 0.3f;

	quads = new QuadArray*[scd->nrChannels];
	pllPositions = new glm::vec3*[scd->nrChannels];
	quadSkeleton = new glm::vec3*[scd->nrChannels];
	quadNormal = new glm::vec3*[scd->nrChannels];
	normSkeleton = new glm::vec3*[scd->nrChannels];
	position = new glm::vec3*[scd->nrChannels];
	normal = new glm::vec3*[scd->nrChannels];
	texCoords = new glm::vec2*[scd->nrChannels];

	for (auto i = 0; i < scd->nrChannels; i++)
	{
//            quads[i] = new QuadArray[nrLines];
		int colInd = i % MAX_NUM_COL_SCENE;
		quads[i] = new QuadArray(nrLines, 1, -1.f, -1.f, 2.f, 2.f,
				chanCols[colInd].r, chanCols[colInd].g, chanCols[colInd].b,
				chanCols[colInd].a); // für texturen hier schwarz

		pllPositions[i] = new glm::vec3[nrLines];
		quadSkeleton[i] = new glm::vec3[nrLines * 2];
		quadNormal[i] = new glm::vec3[nrLines];
		// init first normal
		quadNormal[i][0] = glm::vec3(0.f, 1.f, 0.f);

		normSkeleton[i] = new glm::vec3[nrLines];
		// init first normal
		normSkeleton[i][0] = glm::vec3(0.f, 0.f, 1.f);

		position[i] = new glm::vec3[nrLines * 6];
		normal[i] = new glm::vec3[nrLines * 6];
		texCoords[i] = new glm::vec2[nrLines * 6];
	}
}

//----------------------------------------------------

SNAudioWaveStrip::~SNAudioWaveStrip()
{
}

//----------------------------------------------------

void SNAudioWaveStrip::draw(double time, double dt, camPar* cp,
		Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);
	}


	glClear(GL_DEPTH_BUFFER_BIT);
	sendStdShaderInit(_shader);
	useTextureUnitInd(0, tex0->getId(), _shader, _tfo);

	glm::mat4 m_pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat;
	_shader->setUniformMatrix4fv("modelMatrix", &_modelMat[0][0]);

	for (auto i = 0; i < scd->nrChannels; i++)
		quads[i]->draw(_tfo);
}

//----------------------------------------------------

void SNAudioWaveStrip::update(double time, double dt)
{
	if (lastBlock != pa->waveData.blockCounter)
	{
		updateWave();
		buildQuadSkeleton();
		buildTriangles();
		updateVAO(time);

		lastBlock = pa->waveData.blockCounter;
	}
}

//----------------------------------------------------

void SNAudioWaveStrip::updateWave()
{
	for (auto chanNr=0; chanNr<scd->nrChannels; chanNr++)
	{
		for (int n=0; n<nrLines; n++)
		{
			float fInd = static_cast<float>(n) / static_cast<float>(nrLines - 1);
			for (auto p=0; p<nrPar; p++)
			{
				float val = pa->getPllAtPos(scd->chanMap[chanNr], std::fmod(fInd + pllOffs[p], 1.f));
				signals[p] = val * ampScale;
			}

			// float val = fInd * 2.f - 1.f;
//                pllPositions[chanNr][n] = glm::vec3(val, 0.f, 0.f);
			pllPositions[chanNr][n] = glm::vec3(signals[0], signals[1], signals[2]);
		}
	}
}

//----------------------------------------------------

void SNAudioWaveStrip::buildQuadSkeleton()
{
	for (auto chanNr = 0; chanNr < scd->nrChannels; chanNr++)
	{
		for (auto n = 0; n < nrLines; n++)
		{
			if (n < nrLines - 2)
			{
				glm::vec3 p1p0 = glm::vec3(
						pllPositions[chanNr][n + 1].x
								- pllPositions[chanNr][n].x,
						pllPositions[chanNr][n + 1].y
								- pllPositions[chanNr][n].y,
						pllPositions[chanNr][n + 1].z
								- pllPositions[chanNr][n].z);

				glm::vec3 p1p2 = glm::vec3(
						pllPositions[chanNr][n + 1].x
								- pllPositions[chanNr][n + 2].x,
						pllPositions[chanNr][n + 1].y
								- pllPositions[chanNr][n + 2].y,
						pllPositions[chanNr][n + 1].z
								- pllPositions[chanNr][n + 2].z);

				glm::vec3 normal;

				normal = normalize(cross(normalize(p1p0), normalize(p1p2)));

				// check for nan and inf
				bool isFinite = true;
				if (!std::isfinite(normal.x))
					isFinite = false;
				if (!std::isfinite(normal.y))
					isFinite = false;
				if (!std::isfinite(normal.z))
					isFinite = false;

				// quick and dirty error checking
				// if no normal calculable, take the last valid one
				if (!isFinite)
					normal = quadNormal[chanNr][n];

				quadNormal[chanNr][n + 1] = normal;

				// normale sollte immer "nach oben" zeigen
				//if (normal.y < 0.f) normal = normal * -1.f;
				normal *= lineWidth;

				// smooth normals
				normSkeleton[chanNr][n + 1] = normalize(p1p0 + p1p2); // sollte eigentlich immer zum betrachter zeigen
				// check for nan and inf
				//isFinite = true;
				// wenn die normale nicht finit ist, ist die orthogonale auch nicht finit
				if (!std::isfinite(normSkeleton[chanNr][n + 1].x))
					isFinite = false;
				if (!std::isfinite(normSkeleton[chanNr][n + 1].y))
					isFinite = false;
				if (!std::isfinite(normSkeleton[chanNr][n + 1].z))
					isFinite = false;

				// quick and dirty error checking
				// if no normal calculable, take the last valid one
				if (!isFinite)
				{
					//   std::cout << "n: " << n << " not finite taking from before: "  << glm::to_string(normSkeleton[chanNr][n]) << std::endl;
					normSkeleton[chanNr][n + 1] = normSkeleton[chanNr][n];
				}

				// extrudiere nach "oben"
				quadSkeleton[chanNr][(n + 1) * 2].x =
						pllPositions[chanNr][n + 1].x + normal.x;
				quadSkeleton[chanNr][(n + 1) * 2].y =
						pllPositions[chanNr][n + 1].y + normal.y;
				quadSkeleton[chanNr][(n + 1) * 2].z =
						pllPositions[chanNr][n + 1].z + normal.z;

				// extrudiere nach "unten"
				quadSkeleton[chanNr][(n + 1) * 2 + 1].x = pllPositions[chanNr][n
						+ 1].x - normal.x;
				quadSkeleton[chanNr][(n + 1) * 2 + 1].y = pllPositions[chanNr][n
						+ 1].y - normal.y;
				quadSkeleton[chanNr][(n + 1) * 2 + 1].z = pllPositions[chanNr][n
						+ 1].z - normal.z;

				/*
				 std::cout << "p0: " << glm::to_string(pllPositions[chanNr][n]) << std::endl;
				 std::cout << "p1: " << glm::to_string(pllPositions[chanNr][n+1]) << std::endl;
				 std::cout << "p2: " << glm::to_string(pllPositions[chanNr][n+2]) << std::endl;
				 std::cout << "p1p0: " << glm::to_string(p1p0) << std::endl;
				 std::cout << "p1p2: " << glm::to_string(p1p2) << std::endl;
				 std::cout << "normal: " << glm::to_string(normal) << std::endl;
				 std::cout << "normalSkel: " << glm::to_string(normSkeleton[chanNr][n+1]) << std::endl;
				 std::cout << "quadSkeleton oben: " << glm::to_string(quadSkeleton[chanNr][(n+1)*2]) << std::endl;
				 std::cout << "quadSkeleton unten: " << glm::to_string(quadSkeleton[chanNr][(n+1)*2+1]) << std::endl;
				 std::cout << std::endl;
				 */
			}
		}
	}
}

//----------------------------------------------------

void SNAudioWaveStrip::buildTriangles()
{
	// 0 / 5 *--* 4
	//       |\ |
	//       | \|
	//     1 *--* 2 / 3
	for (auto chanNr = 0; chanNr < scd->nrChannels; chanNr++)
	{
		for (int n = 0; n < nrLines - 1; n++)
		{
			position[chanNr][n * 6] = quadSkeleton[chanNr][n * 2];
			normal[chanNr][n * 6] = normSkeleton[chanNr][n];
			texCoords[chanNr][n * 6] = glm::vec2(0.f, 1.f);

			position[chanNr][n * 6 + 1] = quadSkeleton[chanNr][n * 2 + 1];
			normal[chanNr][n * 6 + 1] = normSkeleton[chanNr][n];
			texCoords[chanNr][n * 6 + 1] = glm::vec2(0.f, 0.f);

			position[chanNr][n * 6 + 2] = quadSkeleton[chanNr][(n + 1) * 2 + 1];
			normal[chanNr][n * 6 + 2] = normSkeleton[chanNr][n + 1];
			texCoords[chanNr][n * 6 + 2] = glm::vec2(1.f, 0.f);

			position[chanNr][n * 6 + 3] = quadSkeleton[chanNr][(n + 1) * 2 + 1];
			normal[chanNr][n * 6 + 3] = normSkeleton[chanNr][n + 1];
			texCoords[chanNr][n * 6 + 3] = glm::vec2(1.f, 0.f);

			position[chanNr][n * 6 + 4] = quadSkeleton[chanNr][(n + 1) * 2];
			normal[chanNr][n * 6 + 4] = normSkeleton[chanNr][n + 1];
			texCoords[chanNr][n * 6 + 4] = glm::vec2(1.f, 1.f);

			position[chanNr][n * 6 + 5] = quadSkeleton[chanNr][n * 2];
			normal[chanNr][n * 6 + 5] = normSkeleton[chanNr][n];
			texCoords[chanNr][n * 6 + 5] = glm::vec2(0.f, 1.f);
		}
	}
}

//----------------------------------------------------

void SNAudioWaveStrip::updateVAO(double time)
{
	glm::vec4 posVec4;
	glm::vec3 normVec3;
	glm::vec3 rotVec = glm::normalize(glm::vec3(0.3f, 0.7f, 0.f));

	for (auto chanNr = 0; chanNr < scd->nrChannels; chanNr++)
	{
		glm::mat4 modMat = glm::rotate(glm::mat4(1.f),
				float(chanNr) / float(scd->nrChannels)
						* float(M_PI * time * 0.1), rotVec);

		glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(modMat)));

		// update positions
		GLfloat* pos = (GLfloat*) quads[chanNr]->getMapBuffer(POSITION);

		for (auto n = 0; n < nrLines; n++)
		{
			for (auto q = 0; q < 6; q++)
			{
				posVec4 = glm::vec4(position[chanNr][n * 6 + q][0],
						position[chanNr][n * 6 + q][1],
						position[chanNr][n * 6 + q][2], 1.f);
				posVec4 = modMat * posVec4;

				for (auto j = 0; j < 3; j++)
				{
					if (n > 1 && (n < nrLines - 3))
					{
						pos[(n * 6 + q) * 3 + j] = posVec4[j];
					}
					else
					{
						pos[(n * 6 + q) * 3 + j] = 0.f;
					}
				}
			}
		}

		quads[chanNr]->unMapBuffer();

		GLfloat* nor = (GLfloat*) quads[chanNr]->getMapBuffer(NORMAL);

		for (auto n = 0; n < nrLines; n++)
		{
			for (auto q = 0; q < 6; q++)
			{
				normVec3 = glm::vec3(normal[chanNr][n * 6 + q][0],
						normal[chanNr][n * 6 + q][1],
						normal[chanNr][n * 6 + q][2]);
				normVec3 = normalMat * normVec3;

				for (int j = 0; j < 3; j++)
				{
					if (n > 1 && (n < nrLines - 3))
					{
						nor[(n * 6 + q) * 3 + j] = normVec3[j];
//                            nor[(n*6 +q)*3 +j] = normal[chanNr][n*6+q][j];
					}
					else
					{
						nor[(n * 6 + q) * 3 + j] = 0.f;
					}
				}
			}
		}

		quads[chanNr]->unMapBuffer();

		GLfloat* texC = (GLfloat*) quads[chanNr]->getMapBuffer(TEXCOORD);

		for (auto n = 0; n < nrLines; n++)
		{
			for (auto q = 0; q < 6; q++)
			{
				for (int j = 0; j < 2; j++)
				{
					if (n > 1 && (n < nrLines - 3))
					{
						texC[(n * 6 + q) * 2 + j] =
								texCoords[chanNr][n * 6 + q][j];
					}
					else
					{
						texC[(n * 6 + q) * 2 + j] = 0.f;
					}
				}
			}
		}

		quads[chanNr]->unMapBuffer();
	}
}

}
