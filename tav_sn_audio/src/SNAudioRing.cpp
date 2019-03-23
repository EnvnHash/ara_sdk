//
// SNAudioRing.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNAudioRing.h"

using namespace glm;
namespace tav
{
SNAudioRing::SNAudioRing(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "LitSphere"), alpha(1.f)
{
	pa = static_cast<PAudio*>(_scd->pa);
	osc = static_cast<OSCData*>(_scd->osc);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	getChanCols(&chanCols);

	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];

	nrXSegments = 250;
	yDistScale = 3.f;
	basicScale = 2.2f;

	ampScale = 4.f;
	ringHeight = 0.3f;

	zOffs = -0.1f;
	yDist = 0.6f;       // verteilung der kanäle über die yPos
	rotSpeed = 0.05f;
	flipNormals = -1.f;
	upperLower = 2;
	incTime = 0.f;

	pllOffs = new glm::vec3[upperLower];
	pllOffs[0] = glm::vec3(0.1f, 0.2f, 0.3f);
	pllOffs[1] = glm::vec3(0.0f, 0.15f, 0.35f);

	qArrays = new QuadArray*[scd->nrChannels];
	pllPositions = new glm::vec3**[scd->nrChannels];
	pllNormals = new glm::vec3**[scd->nrChannels];

	position = new glm::vec3*[scd->nrChannels];
	normal = new glm::vec3*[scd->nrChannels];

	modMatrPerChan = new glm::mat4[scd->nrChannels];

	for (int i = 0; i < scd->nrChannels; i++)
	{
		modMatrPerChan[i] = glm::rotate(
				static_cast<float>(i) / static_cast<float>(scd->nrChannels)
						* static_cast<float>(M_PI), glm::vec3(0.f, 0.9f, 0.1f));
		modMatrPerChan[i][3][1] = (static_cast<float>(i)
				/ static_cast<float>(scd->nrChannels - 1)) * yDist
				- (yDist * 0.5f); // yOffs
		modMatrPerChan[i][3][2] = zOffs;

		//            int colInd = i % MAX_NUM_COL_SCENE;
		//            qArrays[i] = new QuadArray(nrXSegments, 1, -1.f, -1.f, 2.f, 2.f,
		//                                       chanCols[colInd].r, chanCols[colInd].g,
		//                                       chanCols[colInd].b, chanCols[colInd].a);
		qArrays[i] = new QuadArray(nrXSegments, 1, -1.f, -1.f, 2.f, 2.f, 0.f,
				0.f, 0.f, 1.f);

		pllPositions[i] = new glm::vec3*[upperLower];
		for (auto j = 0; j < upperLower; j++)
			pllPositions[i][j] = new glm::vec3[nrXSegments];

		pllNormals[i] = new glm::vec3*[4];
		for (auto j = 0; j < 4; j++)
			pllNormals[i][j] = new glm::vec3[nrXSegments];

		position[i] = new glm::vec3[nrXSegments * 6];
		normal[i] = new glm::vec3[nrXSegments * 6];
	}

	addPar("ampScale", &ampScale);
	addPar("alpha", &alpha);
	addPar("aux1", &aux1);
	addPar("yDist", &yDist);
	addPar("lineHeight", &lineHeight);

}

//----------------------------------------------------

SNAudioRing::~SNAudioRing()
{
	delete pllOffs;
	for (auto i = 0; i < scd->nrChannels; i++)
	{
		delete qArrays[i];
	}
	delete qArrays;
}

//----------------------------------------------------

void SNAudioRing::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);
		_tfo->setSceneNodeColors(chanCols);
	}

	glClear (GL_DEPTH_BUFFER_BIT);
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	sendStdShaderInit(_shader);

	for (int i=0; i<scd->nrChannels; i++)
	{
		modMatrPerChan[i] = glm::rotate(
				static_cast<float>(i) / static_cast<float>(scd->nrChannels)
						* static_cast<float>(M_PI), glm::vec3(0.f, aux1, 0.1f));
		modMatrPerChan[i][3][1] = (static_cast<float>(i)
				/ static_cast<float>(scd->nrChannels - 1)) * yDist
				- (yDist * 0.5f); // yOffs
		modMatrPerChan[i][3][2] = zOffs;
	}

	ringHeight = lineHeight;

	updateWave();
	updateGeo();
	updateVAO(dt);

	useTextureUnitInd(0, tex0->getId(), _shader, _tfo);
	_shader->setUniform1f("oscAlpha", alpha);

	//	glm::mat4 m_pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat;
	_shader->setUniformMatrix4fv("modelMatrix", &_modelMat[0][0]);

	for (auto i = 0; i < scd->nrChannels; i++)
		qArrays[i]->draw(_tfo);
}

//----------------------------------------------------

void SNAudioRing::update(double time, double dt)
{
}

//----------------------------------------------------

void SNAudioRing::updateWave()
{
	for (auto cNr=0; cNr<scd->nrChannels; cNr++){
		for (int n=0; n<nrXSegments; n++){

			float fInd = static_cast<float>(n) / static_cast<float>(nrXSegments - 1);

			for (auto r = 0; r < upperLower; r++)
			{
				float val[3];

				val[0] = pa->getPllAtPos(scd->chanMap[cNr], std::fmod(fInd + pllOffs[r].x, 1.f));
				val[1] = pa->getPllAtPos(scd->chanMap[cNr], std::fmod(fInd + pllOffs[r].y, 1.f));
				val[2] = pa->getPllAtPos(scd->chanMap[cNr], std::fmod(fInd + pllOffs[r].z, 1.f));

				for (auto i = 0; i < 3; i += 2)
					val[i] = (val[i] * ampScale + 1.f) * 0.5f;

				pllPositions[cNr][r][n] = glm::vec3(val[0], val[1], val[2]);
			}
		}
	}
}

//----------------------------------------------------

void SNAudioRing::updateGeo()
{
	// first pass, positions and lower edge normals
	for (auto cNr = 0; cNr < scd->nrChannels; cNr++)
	{
		for (auto n = 0; n < nrXSegments; n++)
		{
			for (int q = 0; q < 6; q++)
			{
				float fInd = 0.f;
				float yDir = 1.f;
				glm::vec3 excent = glm::vec3(0.f);
				int yInd = 0;

				int pllInd = (q > 1 && q < 5) ? n + 1 : n;
				pllInd %= nrXSegments;
				fInd = static_cast<float>(pllInd)
						/ static_cast<float>(nrXSegments);  // quad rechte seite

				if (q > 0 && q < 4)
				{
					yDir = -1.f;                            // quad untere seite
					excent = pllPositions[cNr][0][pllInd], yInd = 0;
				}
				else
				{
					excent = pllPositions[cNr][1][pllInd];  // quad obere seite
					yInd = 1;
				}

				position[cNr][n * 6 + q].x = std::cos(fInd * TWO_PI)
						* basicScale * excent.x;
				position[cNr][n * 6 + q].y =
						(ringHeight + excent.y * yDistScale) * yDir;
				position[cNr][n * 6 + q].z = std::sin(fInd * TWO_PI)
						* basicScale * excent.x;
			}

			// berechne das kreuzprodukt an der linken unteren ecke des quads
			// komischerweise hier mit * -1 falsches ergebnis...
			glm::vec3 p1p0 = position[cNr][n * 6 + 1] - position[cNr][n * 6];
			glm::vec3 p1p2 = position[cNr][n * 6 + 1]
					- position[cNr][n * 6 + 2];
			pllNormals[cNr][0][n] = cross(p1p0, p1p2);

			// berechne das kreuzprodukt an der rechten unteren ecke des quads
			glm::vec3 p2p1 = position[cNr][n * 6 + 2]
					- position[cNr][n * 6 + 1];
			glm::vec3 p2p4 = position[cNr][n * 6 + 2]
					- position[cNr][n * 6 + 4];
			pllNormals[cNr][1][n] = cross(p2p1, p2p4);

			// bei mehreren Y-Segmenten könnte dieser schritt ab der zweiten
			// reihe übersprungen werden... ergebnis wäre nur unwesentlich anders

			// berechne das kreuzprodukt an der rechten oberen ecke des quads
			// wird in die falsche richtung zeigen, deshalb * -1;
			glm::vec3 p4p0 = position[cNr][n * 6 + 4] - position[cNr][n * 6];
			glm::vec3 p4p3 = position[cNr][n * 6 + 4]
					- position[cNr][n * 6 + 3];
			pllNormals[cNr][2][n] = cross(p4p0, p4p3) * -1.f;

			// berechne das kreuzprodukt an der linken oberen ecke des quads
			glm::vec3 p0p4 = position[cNr][n * 6] - position[cNr][n * 6 + 4];
			glm::vec3 p0p1 = position[cNr][n * 6] - position[cNr][n * 6 + 1];
			pllNormals[cNr][3][n] = cross(p0p4, p0p1);

			//                for (int k=0;k<4;k++)
			//                    std::cout << "normal [" << k << "]: " <<  glm::to_string(pllNormals[cNr][k][n]) << std::endl;
			//                std::cout << std::endl;

			// wenn n grösser als 1 bilde für die linke untere ecke des quads
			// einen mittelwert mit der rechten unteren Kante des vorigen Segments
			// später werden für die linke untere und rechte untere ecke nur
			// dieser wert genommen (index:0)
			if (n > 0)
			{
				pllNormals[cNr][0][n] = normalize(
						(pllNormals[cNr][0][n] + pllNormals[cNr][1][n - 1])
								* 0.5f) * flipNormals;

				// für die erste reihe der y-segmente auch die oberen ecken interpolieren
				pllNormals[cNr][3][n] = normalize(
						(pllNormals[cNr][3][n] + pllNormals[cNr][2][n - 1])
								* 0.5f) * flipNormals;
			}
		}
	}
}

//----------------------------------------------------

void SNAudioRing::updateVAO(double _dt)
{
	//dt = _time - lastTime;
	incTime += _dt * osc->speed;

	for (auto cNr = 0; cNr < scd->nrChannels; cNr++)
	{
		// change color
		//float alpha = std::fmin(pa->getMedAmp(scd->chanMap[cNr]) * 8.f, 1.f);
		float alpha = 1;
		int cnInd = cNr % MAX_NUM_COL_SCENE;

		qArrays[cNr]->setColor(std::fmin(chanCols[cnInd].r, 1.f),
				std::fmin(chanCols[cnInd].g, 1.f),
				std::fmin(chanCols[cnInd].b, 1.f),
				std::fmin(chanCols[cnInd].a * alpha, 1.f));

		// update modMatr with continous rotation
		glm::mat4 rotMat = modMatrPerChan[cNr]
				* glm::rotate(static_cast<float>(M_PI) * 0.2f,
						glm::vec3(1.f, 0.f, 0.f))
				* glm::rotate(
						static_cast<float>(std::fmod(incTime * rotSpeed, 1.0f))
								* static_cast<float>(M_PI) * 2.f,
						glm::vec3(0.f, 1.f, 0.f));

		// update positions
		GLfloat* pos = (GLfloat*) qArrays[cNr]->getMapBuffer(POSITION);

		for (int n = 0; n < nrXSegments; n++)
		{
			for (auto q = 0; q < 6; q++)
			{
				glm::vec4 modvec = rotMat
						* glm::vec4(position[cNr][n * 6 + q], 1.f);
				pos[(n * 6 + q) * 3] = modvec.x;
				pos[(n * 6 + q) * 3 + 1] = modvec.y;
				pos[(n * 6 + q) * 3 + 2] = modvec.z;
			}
		}

		qArrays[cNr]->unMapBuffer();

		GLfloat* nor = (GLfloat*) qArrays[cNr]->getMapBuffer(NORMAL);

		for (int n = 1; n < nrXSegments; n++)
		{
			glm::vec3 modNor1 = glm::mat3(rotMat) * pllNormals[cNr][3][n];
			glm::vec3 modNor2 = glm::mat3(rotMat) * pllNormals[cNr][0][n];

			// links oben das vorigen quads
			nor[(n * 6) * 3] = modNor1.x;
			nor[(n * 6) * 3 + 1] = modNor1.y;
			nor[(n * 6) * 3 + 2] = modNor1.z;

			// links oben das vorigen quads
			nor[(n * 6 + 5) * 3] = modNor1.x;
			nor[(n * 6 + 5) * 3 + 1] = modNor1.y;
			nor[(n * 6 + 5) * 3 + 2] = modNor1.z;

			//  setze die rechte obere des vorigen mit
			// rechts oben das aktuellen quads
			nor[((n - 1) * 6 + 4) * 3] = modNor1.x;
			nor[((n - 1) * 6 + 4) * 3 + 1] = modNor1.y;
			nor[((n - 1) * 6 + 4) * 3 + 2] = modNor1.z;

			// links unten das vorigen quads
			nor[(n * 6 + 1) * 3] = modNor2.x;
			nor[(n * 6 + 1) * 3 + 1] = modNor2.y;
			nor[(n * 6 + 1) * 3 + 2] = modNor2.z;

			//  setze die rechte untere des vorigen mit
			// rechts unten das aktuellen quads
			nor[((n - 1) * 6 + 2) * 3] = modNor2.x;
			nor[((n - 1) * 6 + 2) * 3 + 1] = modNor2.y;
			nor[((n - 1) * 6 + 2) * 3 + 2] = modNor2.z;

			// rechts unten das aktuellen quads
			nor[((n - 1) * 6 + 3) * 3] = modNor2.x;
			nor[((n - 1) * 6 + 3) * 3 + 1] = modNor2.y;
			nor[((n - 1) * 6 + 3) * 3 + 2] = modNor2.z;
		}

		qArrays[cNr]->unMapBuffer();
	}
}

}
