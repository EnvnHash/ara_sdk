//
//  GLSLHistogram.hpp
//  tav_core
//
//  Created by Sven Hahne on 08/06/16.
//  Copyright © 2016 Sven Hahne. All rights reserved..
//

#ifndef GLSLHistogram_hpp
#define GLSLHistogram_hpp

#pragma once

#include <stdio.h>
#include <math.h>

#include <glm/glm.hpp>

#include "GLUtils/PingPongFbo.h"
#include "GLUtils/FBO.h"
#include "GeoPrimitives/Quad.h"
#include "Shaders/ShaderCollector.h"

namespace tav
{
class GLSLHistogram
{
public:
	GLSLHistogram(ShaderCollector* _shCol, int _width, int _height,
			GLenum _type, unsigned int _downSample, unsigned int _histWidth = 512,
			bool _getBounds = true, bool _normalize = true,
			float _maxValPerChan = 1.f);
	~GLSLHistogram();
	void procMinMax(GLint texId);
	void proc(GLint texId);
	void downloadMinMax();
	void downloadEnergyCenter();

	float getMaximum(unsigned int chan = 0);
	float getMinimum(unsigned int chan = 0);
	float getMaxInd(unsigned int chan = 0);
	float getMinInd(unsigned int chan = 0);
	float getHistoPeakInd(unsigned int chan = 0);
	GLint getResult();
	GLint getMinMaxTex();
	float getSubtrMax();
	float getEnergySum(float lowThresh);

	void setSmoothing(float _val);
	void setValThres(float _val);
	void setIndValThres(float _val);

	void initShader();
	void initNormShader();
	void initMinMaxShader();
	void initMinMaxSpectrShader();
	void initEnergyMedShader();

private:
	ShaderCollector* shCol;
	Shaders* histoShader;
	Shaders* minMaxShader;
	Shaders* minMaxSpectrShader;
	Shaders* energyMedShader;
	Shaders* normShader;

	FBO* minMaxFbo;
	FBO* minMaxSpectrFbo;
	FBO* energyMedFbo;
	PingPongFbo* histoFbo;

	VAO* trigVao;
	VAO* trigSpectrVao;
	Quad* quad;

	GLenum minMaxType;
	GLenum minMaxFormat;
	GLenum texType;
	GLenum format;
	GLenum texFormat;

	GLint geoAmp;
	GLfloat* minMax;
	GLfloat* minMaxSpectr;
	GLfloat* energyMed;
	GLfloat* histoDownload;

	int width;
	int height;
	int totNrCells;
	int maxHistoWidth;
	int nrChan;
	int spectrNrEmitTrig;
	int spectrGeoAmp;
	unsigned int downSample;
	int procFrame = 0;
	int lastDownloadFrame = 0;
	int lastEnerDownloadFrame = 0;
	int procEnerFrame = 0;

	bool normalize;
	bool getBounds;
	bool getEnergyCenter = false;
	bool b_getEnergySum=false;

	float valThres;
	float indValThres;
	float maxValOfType;
	float maxValPerChan;
	float maxValSum;
	float maximum = 0.f;
	float minimum = 0.f;
	float spectrMax = 0.f;
	float spectrMin = 0.f;
	float energySum = 0.f;
	float energySumLowThresh = 0.1f;
	int spectrMaxInd = 0.f;
	int spectrMinInd = 0.f;

	float smoothing = 0.f;
	glm::ivec2 cellSize;
	glm::ivec2 nrCells;
	glm::ivec2 histoTexSize;
};
}

#endif /* GLSLHistogram_hpp */
