//
//  AudioTexture.cpp
//  tav_core
//
//  Created by Sven Hahne on 26/12/15.
//  Copyright © 2015 Sven Hahne. All rights reserved.
//

#define STRINGIFY(A) #A

#include "pch.h"
#include "AudioTexture.h"

namespace tav
{

AudioTexture::AudioTexture(ShaderCollector* _shCol, unsigned int _nrChannels,
		unsigned int _frames_per_buffer) :
		shCol(_shCol), nrChannels(_nrChannels), frames_per_buffer(
				_frames_per_buffer), memSize(2), frameNr(-1), maxNrChannels(8)
{
	audioTex = new TextureManager**[nrChannels];
	memPtr = new unsigned int[nrChannels];
	audioTexUnits = new int[nrChannels];
	for (unsigned int i = 0; i < nrChannels; i++)
	{
		audioTex[i] = new TextureManager*[memSize];
		audioTexUnits[i] = i;
		for (unsigned int j = 0; j < memSize; j++)
		{
			audioTex[i][j] = new TextureManager();
			audioTex[i][j]->allocate(_frames_per_buffer, GL_R16F, GL_RED);
		}
		memPtr[i] = 0;
	}

	audioTex2D = new FBO*[memSize];
	audioTex2DNrChans = int(float(nrChannels / 2) + 0.5f) * 2; // make sure it´s power of two
	fAudioTex2DNrChans = float(audioTex2DNrChans);
	for (unsigned int j = 0; j < memSize; j++)
		audioTex2D[j] = new FBO(shCol, _frames_per_buffer, audioTex2DNrChans,
				GL_R16F,
				GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false);

	quad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 1.f);
	initCopyShdr();
}

//---------------------------------------------------------------

void AudioTexture::update(float** _data, int _frameNr)
{
	if (frameNr != _frameNr)
	{
		for (unsigned int i = 0; i < nrChannels; i++)
		{
			memPtr[i] = (memPtr[i] + 1) % memSize;

			glActiveTexture(GL_TEXTURE0);
			audioTex[i][memPtr[i]]->bind();

			glTexSubImage1D(GL_TEXTURE_1D,             // target
					0,                          // First mipmap level
					0,                       // x offset
					frames_per_buffer,
					GL_RED,
					GL_FLOAT, _data[i]);
		}
		frameNr = _frameNr;
	}
}


//---------------------------------------------------------------

void AudioTexture::update(float* _data, int _chanNr)
{
	memPtr[_chanNr] = (memPtr[_chanNr] + 1) % memSize;

	glActiveTexture(GL_TEXTURE0);
	audioTex[_chanNr][memPtr[_chanNr]]->bind();

	glTexSubImage1D(GL_TEXTURE_1D,   // target
			0,                       // First mipmap level
			0,                       // x offset
			frames_per_buffer,
			GL_RED,
			GL_FLOAT, _data);
}

//---------------------------------------------------------------

GLuint AudioTexture::getTex(unsigned int chanNr)
{
	return audioTex[chanNr][memPtr[chanNr]]->getId();
}

//---------------------------------------------------------------

GLuint AudioTexture::getTex2D()
{
	return audioTex2D[memPtr[0]]->getColorImg();
}

//---------------------------------------------------------------

void AudioTexture::bindTex(unsigned int chanNr)
{
	glBindTexture(GL_TEXTURE_1D, audioTex[chanNr][memPtr[chanNr]]->getId());
}

//---------------------------------------------------------------

void AudioTexture::bindLastTex(unsigned int chanNr)
{
	glBindTexture(GL_TEXTURE_1D,
			audioTex[chanNr][(memPtr[chanNr] - 1 + memSize) % memSize]->getId());
}

//---------------------------------------------------------------

void AudioTexture::mergeTo2D(int _frameNr)
{
	if (lastMergeFrameNr != _frameNr)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);

		audioTex2D[memPtr[0]]->bind();

		copyShdr->begin();
		copyShdr->setIdentMatrix4fv("m_pvm");
		copyShdr->setUniform1iv("audioTex", audioTexUnits, std::min(nrChannels, maxNrChannels));
		copyShdr->setUniform1f("nrChannels", fAudioTex2DNrChans);

		for (unsigned int i = 0; i < std::min(nrChannels, maxNrChannels); i++)
			audioTex[i][memPtr[i]]->bind(i);

		quad->draw();

		audioTex2D[memPtr[0]]->unbind();

		lastMergeFrameNr = _frameNr;
	}
}

//--------------------------------------------------------------------------------

void AudioTexture::initCopyShdr()
{
	//------ Position Shader -----------------------------------------

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position;\n layout (location=2) in vec2 texCoord;\n uniform mat4 m_pvm;\n out vec4 pos;\n out vec2 texCo;\n

					void main(void) {\n pos = position;\n texCo = texCoord;\n gl_Position = m_pvm * position;\n });
	vert = "// AudioTexture copy Shader vertex shader\n" + shdr_Header + vert;

	std::string frag = "layout(location = 0) out vec4 fragColor;\n";
	frag += "uniform sampler1D audioTex["
			+ std::to_string(std::min(nrChannels, maxNrChannels)) + "];\n";

	std::string fragBody =
			STRINGIFY(
					in vec4 pos;\n in vec2 texCo;\n uniform float nrChannels;\n void main()\n { fragColor = texture(audioTex[int(texCo.y * nrChannels)], texCo.x );\n });
	frag = "// AudioTexture copy Shader shader\n" + shdr_Header + frag
			+ fragBody;

	copyShdr = shCol->addCheckShaderText("audioTextureCopyShdr", vert.c_str(),
			frag.c_str());
}

//---------------------------------------------------------------

AudioTexture::~AudioTexture()
{
	for (unsigned int i = 0; i < nrChannels; i++)
		delete[] audioTex[i];

	delete[] audioTex;
	delete[] memPtr;
}
}
