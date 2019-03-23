//
// SNAudioWaveStrip2.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNAudioWaveStrip2.h"

using namespace glm;

namespace tav
{

SNAudioWaveStrip2::SNAudioWaveStrip2(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "LitSphere"), ampScale(1.f), alpha(1.f)
{
	pa = (PAudio*) scd->pa;
	osc = (OSCData*) scd->osc;
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	getChanCols(&chanCols);

	nrLines = 600;
//        nrLines = 400;
	lineWidth = 3.f;
	minLineWidth = 0.01f;

	nrPar = 3;
	signals = new float[nrPar];
	for (auto i=0; i<nrPar; i++)
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
		quads[i] = new QuadArray(nrLines, 1, -1.f, -1.f, 2.f, 2.f,
				chanCols[i].r, chanCols[i].g, chanCols[i].b, chanCols[i].a); // für texturen hier schwarz

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

	addPar("alpha", &alpha);
	addPar("aux1", &aux1);
	addPar("lineHeight", &lineHeight);
	addPar("ampScale", &ampScale);
}

//----------------------------------------------------

SNAudioWaveStrip2::~SNAudioWaveStrip2()
{
	delete quads;
	delete pllPositions;
	delete quadSkeleton;
	delete quadNormal;
	delete normSkeleton;
	delete position;
	delete normal;
	delete texCoords;
}

//----------------------------------------------------

void SNAudioWaveStrip2::draw(double time, double dt, camPar* cp,
		Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);
	}

	sendStdShaderInit(_shader);

	if (lastBlock != pa->waveData.blockCounter)
	{
		updateWave();
		buildQuadSkeleton();
		buildTriangles();
		updateVAO();

		lastBlock = pa->waveData.blockCounter;
	}

	glEnable (GL_BLEND);
	//   glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClear (GL_DEPTH_BUFFER_BIT);

	lineWidth = 3.f + aux1 * 10.f + lineHeight;

	_shader->setUniform1f("oscAlpha", alpha);
//	_shader->setUniform1f("oscAlpha", alpha * osc->alpha);

	glm::mat4 m_pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat;
	_shader->setUniformMatrix4fv("modelMatrix", &_modelMat[0][0]);

	for (auto i = 0; i < scd->nrChannels; i++)
		quads[i]->draw(_tfo);
}

//----------------------------------------------------

void SNAudioWaveStrip2::update(double time, double dt)
{
}

//----------------------------------------------------

void SNAudioWaveStrip2::updateWave()
{
	for (int chanNr=0; chanNr<scd->nrChannels; chanNr++){
		for (int n=0; n<nrLines; n++)
		{
			float fInd = static_cast<float>(n) / static_cast<float>(nrLines - 1);

			for (int p=0; p<nrPar; p++)
				signals[p] = pa->getPllAtPos(scd->chanMap[chanNr], std::fmod(fInd + pllOffs[p], 1.f)) * ampScale;

			pllPositions[chanNr][n] = glm::vec3(signals[0], signals[1], signals[2] - (0.5f * (1.f + ampScale)));
		}
	}
}

//----------------------------------------------------

void SNAudioWaveStrip2::buildQuadSkeleton()
{
	for (auto chanNr = 0; chanNr < scd->nrChannels; chanNr++)
	{
		for (auto n = 0; n < nrLines; n++)
		{
			if (n < nrLines - 2)
			{
				glm::vec3 p1p0 = glm::vec3(
						pllPositions[chanNr][n + 1].x - pllPositions[chanNr][n].x,
						pllPositions[chanNr][n + 1].y - pllPositions[chanNr][n].y,
						pllPositions[chanNr][n + 1].z - pllPositions[chanNr][n].z);

				glm::vec3 p1p2 = glm::vec3(
						pllPositions[chanNr][n + 1].x - pllPositions[chanNr][n + 2].x,
						pllPositions[chanNr][n + 1].y - pllPositions[chanNr][n + 2].y,
						pllPositions[chanNr][n + 1].z - pllPositions[chanNr][n + 2].z);

				glm::vec3 normal;

				normal = normalize(cross(normalize(p1p0), normalize(p1p2)));

				// check for nan and inf
				bool isFinite = true;
				if (!std::isfinite(normal.x)) isFinite = false;
				if (!std::isfinite(normal.y)) isFinite = false;
				if (!std::isfinite(normal.z)) isFinite = false;

				// quick and dirty error checking
				// if no normal calculable, take the last valid one
				if (!isFinite) normal = quadNormal[chanNr][n];

				quadNormal[chanNr][n + 1] = normal;

				// normale sollte immer "nach oben" zeigen
				//if (normal.y < 0.f) normal = normal * -1.f;
				normal *= lineWidth * pow( fabs(
												pa->getPllAtPos(
														scd->chanMap[chanNr],
														static_cast<float>(n)
																/ static_cast<float>(nrLines
																		- 1)
																* 0.4f + 0.2f)),
										1.5f) + minLineWidth;

				// smooth normals
				normSkeleton[chanNr][n + 1] = normalize(p1p0 + p1p2); // sollte eigentlich immer zum betrachter zeigen
				// check for nan and inf
				//isFinite = true;
				// wenn die normale nicht finit ist, ist die orthogonale auch nicht finit
				if (!std::isfinite(normSkeleton[chanNr][n + 1].x)) isFinite = false;
				if (!std::isfinite(normSkeleton[chanNr][n + 1].y)) isFinite = false;
				if (!std::isfinite(normSkeleton[chanNr][n + 1].z)) isFinite = false;

				// quick and dirty error checking
				// if no normal calculable, take the last valid one
				if (!isFinite)
				{
					//   std::cout << "n: " << n << " not finite taking from before: "  << glm::to_string(normSkeleton[chanNr][n]) << std::endl;
					normSkeleton[chanNr][n + 1] = normSkeleton[chanNr][n];
				}

				// normale soll immer zu, betrachter zeigen
				//if ( normSkeleton[chanNr][n+1].z < 0.f ) normSkeleton[chanNr][n+1] = normSkeleton[chanNr][n+1] * -1.f;

				// extrudiere nach "oben"
				quadSkeleton[chanNr][(n + 1) * 2].x = pllPositions[chanNr][n + 1].x + normal.x;
				quadSkeleton[chanNr][(n + 1) * 2].y = pllPositions[chanNr][n + 1].y + normal.y;
				quadSkeleton[chanNr][(n + 1) * 2].z = pllPositions[chanNr][n + 1].z + normal.z;

				// extrudiere nach "unten"
				quadSkeleton[chanNr][(n + 1) * 2 + 1].x = pllPositions[chanNr][n + 1].x - normal.x;
				quadSkeleton[chanNr][(n + 1) * 2 + 1].y = pllPositions[chanNr][n + 1].y - normal.y;
				quadSkeleton[chanNr][(n + 1) * 2 + 1].z = pllPositions[chanNr][n + 1].z - normal.z;

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

void SNAudioWaveStrip2::buildTriangles()
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

void SNAudioWaveStrip2::updateVAO()
{
	for (auto chanNr = 0; chanNr < scd->nrChannels; chanNr++)
	{
		// update positions
		GLfloat* pos = (GLfloat*) quads[chanNr]->getMapBuffer(POSITION);

		for (auto n = 0; n < nrLines; n++)
			for (auto q = 0; q < 6; q++)
				for (auto j = 0; j < 3; j++)
					if (n > 1 && (n < nrLines - 3))
						pos[(n * 6 + q) * 3 + j] = position[chanNr][n * 6 + q][j];
					else
						pos[(n * 6 + q) * 3 + j] = 0.f;

		quads[chanNr]->unMapBuffer();

		GLfloat* nor = (GLfloat*) quads[chanNr]->getMapBuffer(NORMAL);

		for (auto n = 0; n < nrLines; n++)
			for (auto q = 0; q < 6; q++)
				for (int j = 0; j < 3; j++)
					if (n > 1 && (n < nrLines - 3))
						nor[(n * 6 + q) * 3 + j] = normal[chanNr][n * 6 + q][j];
					else
						nor[(n * 6 + q) * 3 + j] = 0.f;

		quads[chanNr]->unMapBuffer();

		GLfloat* texC = (GLfloat*) quads[chanNr]->getMapBuffer(TEXCOORD);

		for (auto n = 0; n < nrLines; n++)
			for (auto q = 0; q < 6; q++)
				for (int j = 0; j < 2; j++)
					if (n > 1 && (n < nrLines - 3))
						texC[(n * 6 + q) * 2 + j] = texCoords[chanNr][n * 6 + q][j];
					else
						texC[(n * 6 + q) * 2 + j] = 0.f;

		quads[chanNr]->unMapBuffer();
	}
}

}
