//
//  AudioTexture.hpp
//  tav_core
//
//  Created by Sven Hahne on 26/12/15.
//  Copyright © 2015 Sven Hahne. All rights reserved..
//

#ifndef AudioTexture_hpp
#define AudioTexture_hpp

#pragma once

#include <stdio.h>
#include "GLUtils/FBO.h"
#include "GeoPrimitives/Quad.h"
#include "Shaders/ShaderCollector.h"
#include "Shaders/Shaders.h"
#include "GLUtils/TextureManager.h"

namespace tav
{
class AudioTexture
{
public:
	AudioTexture(ShaderCollector* _shCol, unsigned int _nrChannels,
			unsigned int _frames_per_buffer);
	~AudioTexture();
	void update(float** _data, int _frameNr);
	void update(float* _data, int _chanNr);
	GLuint getTex(unsigned int chanNr);
	GLuint getTex2D();
	void bindTex(unsigned int chanNr);
	void bindLastTex(unsigned int chanNr);
	void mergeTo2D(int _frameNr);
	void initCopyShdr();

private:
	TextureManager*** audioTex;
	FBO** audioTex2D;

	ShaderCollector* shCol;
	Shaders* copyShdr;
	Quad* quad;

	unsigned int nrChannels;
	unsigned int maxNrChannels;
	unsigned int audioTex2DNrChans;
	unsigned int frames_per_buffer;
	unsigned int memSize;
	unsigned int* memPtr;
	int* audioTexUnits;

	int frameNr;
	int lastMergeFrameNr;

	float fAudioTex2DNrChans;
};
}

#endif /* AudioTexture_hpp */
