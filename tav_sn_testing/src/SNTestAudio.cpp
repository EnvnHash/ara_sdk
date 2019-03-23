//
// SNTestAudio.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//

#include "SNTestAudio.h"

using namespace std;

namespace tav
{
SNTestAudio::SNTestAudio(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
#ifdef HAVE_AUDIO

	pa = (PAudio*) scd->pa;

	colShader = shCol->getStdCol();

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	// rawWave
	audioPar.push_back(new testAudioPar());
	audioPar.back()->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
	audioPar.back()->chanYOffs = 0.f;
	audioPar.back()->getVal = &SNTestAudio::sampData;

	// smooth FFT mit "linearen" frequenzen
	audioPar.push_back(new testAudioPar());
	audioPar.back()->color = glm::vec4(1.f, 0.f, 0.f, 1.f);
	audioPar.back()->chanYOffs = 1.f / static_cast<float>(nrPar);
	audioPar.back()->getVal = &SNTestAudio::fftData;

	// pll
	audioPar.push_back(new testAudioPar());
	audioPar.back()->color = glm::vec4(0.f, 1.f, 0.f, 1.f);
	audioPar.back()->chanYOffs = 2.f / static_cast<float>(nrPar);
	audioPar.back()->getVal = &SNTestAudio::pllData;

	// pitch
	audioPar.push_back(new testAudioPar());
	audioPar.back()->color = glm::vec4(0.f, 0.f, 1.f, 1.f);
	audioPar.back()->chanYOffs = 3.f / static_cast<float>(nrPar);
	audioPar.back()->getVal = &SNTestAudio::pitch;

	nrPar = static_cast<int>(audioPar.size());

	// onset
	//        audioPar.push_back(new testAudioPar());
	//        audioPar.back()->color = glm::vec4(1.f, 0.f, 1.f, 1.f);
	//        audioPar.back()->chanYOffs = 4.f / static_cast<float>(nrPar);
	//        audioPar.back()->getVal = &SNTestAudio::onset;

	lines = new Line**[audioPar.size()];

	for (auto i=0;i<audioPar.size();i++)
	{
		lines[i] = new Line*[scd->nrChannels];

		for (auto j=0;j<scd->nrChannels;j++)
			lines[i][j] = new Line(pa->getFramesPerBuffer(),
					audioPar[i]->color.r, audioPar[i]->color.g,
					audioPar[i]->color.b, audioPar[i]->color.a);
	}
#endif
}

//----------------------------------------------------

void SNTestAudio::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		glDisable(GL_RASTERIZER_DISCARD);
		_tfo->end();
	}
	colShader->begin();
	colShader->setIdentMatrix4fv("m_pvm");

	glDisable(GL_DEPTH_TEST);

	// clear screen
	quad->draw();

	std::cout << scd->nrChannels << std::endl;

#ifdef HAVE_AUDIO

	if ( lastBlock != pa->getBlockCount() )
	{
		for (auto i=0;i<scd->nrChannels;i++)
			for (auto j=0;j<audioPar.size();j++)
				updateWave(j, i);

		lastBlock = pa->getBlockCount();
	}


	for (auto i=0;i<scd->nrChannels;i++)
		for (auto j=0;j<audioPar.size();j++)
			lines[j][i]->draw(GL_LINE_STRIP);

#endif

	if (_tfo)
	{
		glEnable(GL_RASTERIZER_DISCARD);
		_shader->begin();       // extrem wichtig, sonst keine Bindepunkte für TFO!!!
		_tfo->begin(_tfo->getLastMode());
	}
}

//----------------------------------------------------

void SNTestAudio::update(double time, double dt)
{}

//----------------------------------------------------
#ifdef HAVE_AUDIO

void SNTestAudio::updateWave(int parNr, int chanNr)
{
	std::cout << "draw" << std::endl;

	GLfloat yOffs = static_cast<GLfloat>(parNr + chanNr * nrPar)
                        		/ static_cast<GLfloat>(scd->nrChannels * nrPar -1);         // 0 | 1
	yOffs += (1.f / static_cast<GLfloat>(scd->nrChannels * nrPar -1)) * 0.5f;   // offset half line
	yOffs = yOffs * 2.f - 1.f;                                                  // -1 | 1
	yOffs *= -1.f;                                                              // invert

	GLfloat* pos = (GLfloat*) lines[parNr][chanNr]->getMapBuffer(POSITION);

	// Set model matrices for each instance
	for (int n=0; n<lines[parNr][chanNr]->nrSegments; n++)
	{
		float fInd = static_cast<float>(n) / static_cast<float>(lines[parNr][chanNr]->nrSegments-1);
		glm::vec3 newPos = glm::vec3(fInd * 2.f - 1.0f,
				audioPar[parNr]->getVal(pa, chanNr, fInd) + yOffs,
				0.f);
		for (int j=0;j<3;j++) pos[n*3+j] = newPos[j];
	}

	lines[parNr][chanNr]->unMapBuffer();
}

//----------------------------------------------------

float SNTestAudio::sampData(PAudio* _pa,int chanNr, float fInd)
{
	return _pa->getSampDataAtPos(chanNr, fInd);
}

//----------------------------------------------------

float SNTestAudio::fftData(PAudio* _pa, int chanNr, float fInd)
{
	return _pa->getSmoothSpectrAt(chanNr, fInd);
}

//----------------------------------------------------

float SNTestAudio::pllData(PAudio* _pa, int chanNr, float fInd)
{
	return _pa->getPllAtPos(chanNr, fInd);
}

//----------------------------------------------------

float SNTestAudio::pitch(PAudio* _pa, int chanNr, float fInd)
{
	return _pa->getPitch(chanNr) / 8000.f;
}

//----------------------------------------------------

float SNTestAudio::onset(PAudio* _pa, int chanNr, float fInd)
{
	return _pa->getOnset(chanNr);
}
#endif
//----------------------------------------------------

SNTestAudio::~SNTestAudio()
{}
}
