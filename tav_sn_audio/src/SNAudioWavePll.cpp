//
// SNAudioWavePll.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Uses Lines... dificult for blending...
//

#include "SNAudioWavePll.h"

namespace tav
{
SNAudioWavePll::SNAudioWavePll(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs)
{
	pa = (PAudio*) scd->pa;
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	getChanCols(&chanCols);

	colShader = shCol->getStdCol();

	int nrLines = 512;
//        int nrLines = 30;
	ampScale = 8.f;

	nrPar = 3;
	signals = new float[nrPar];
	for (auto i = 0; i < nrPar; i++)
		signals[i] = 0.f;

	pllOffs = new float[nrPar];
	pllOffs[0] = 0.1f;
	pllOffs[1] = 0.2f;
	pllOffs[2] = 0.3f;

	lines = new Line*[scd->nrChannels];
	for (auto i = 0; i < scd->nrChannels; i++)
		lines[i] = new Line(nrLines);
}

//----------------------------------------------------

SNAudioWavePll::~SNAudioWavePll()
{
}

//----------------------------------------------------

void SNAudioWavePll::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		glDisable (GL_RASTERIZER_DISCARD);
		_tfo->end();
	}
//        sendStdShaderInit(_shader);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ohne dass kein antialiasing

	colShader->begin();
	colShader->setIdentMatrix4fv("m_pvm");

	if (lastBlock != pa->waveData.blockCounter)
	{
		updateWave();
		lastBlock = pa->waveData.blockCounter;
	}

	glDepthMask (GL_FALSE);  // nötig um die scharzen Ränder zu eliminieren

	for (auto i = 0; i < scd->nrChannels; i++)
		lines[i]->draw(GL_LINE_STRIP);

	glDepthMask (GL_TRUE);

	if (_tfo)
	{
		glEnable (GL_RASTERIZER_DISCARD);
		_shader->begin();  // extrem wichtig, sonst keine Bindepunkte für TFO!!!
		_tfo->begin(_tfo->getLastMode());
	}
}

//----------------------------------------------------

void SNAudioWavePll::update(double time, double dt)
{
}

//----------------------------------------------------

void SNAudioWavePll::updateWave()
{
	for (auto chanNr = 0; chanNr < scd->nrChannels; chanNr++)
	{
		GLfloat* pos = (GLfloat*) lines[chanNr]->getMapBuffer(POSITION);

		// Set model matrices for each instance
		for (int n = 0; n < lines[chanNr]->nrSegments; n++)
		{
			float fInd = static_cast<float>(n)
					/ static_cast<float>(lines[chanNr]->nrSegments - 1);
			for (auto p = 0; p < nrPar; p++)
			{
//                    float val = pa->getSampDataAtPos(scd->chanMap[chanNr], std::fmod(fInd + pllOffs[p], 1.f));
				float val = pa->getPllAtPos(scd->chanMap[chanNr],
						std::fmod(fInd + pllOffs[p], 1.f));
				signals[p] = val * ampScale;
			}

			glm::vec3 newPos = glm::vec3(signals[0], signals[1], signals[2]);

			for (int j = 0; j < 3; j++)
				pos[n * 3 + j] = newPos[j];
		}
		lines[chanNr]->unMapBuffer();
	}
}

}
