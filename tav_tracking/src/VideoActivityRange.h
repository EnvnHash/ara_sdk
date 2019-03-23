/*
 * VideoActivityRange.h
 *
 *  Created on: 25.01.2017
 *      Copyright by Sven Hahne
 */

#ifndef VIDEOACTIVITYRANGE_H_
#define VIDEOACTIVITYRANGE_H_

#pragma once

#include <iostream>

#include "OpenCvUtils/CvKalmanFilter.h"
#include "GLUtils/GLSL/GLSLHistogram.h"
#include "GLUtils/GLSL/GLSLTimeMedian.h"
#include "GLUtils/FBO.h"
#include "GLUtils/GLSL/FastBlurMem.h"
#include "Median.h"
#include "Shaders/ShaderCollector.h"
#include "Shaders/Shaders.h"
#include "GLUtils/TextureManager.h"

#include <vector>

namespace tav
{

class VideoActivityRange
{
public:
	VideoActivityRange(ShaderCollector* _shCol, unsigned int _width,
			unsigned int _height, float _dstRatio = 1.f, float _minSize = 0.4f);
	virtual ~VideoActivityRange();

	void initDiffShdr(ShaderCollector* _shCol);
	void initPosColShdr(ShaderCollector* _shCol);
	void initHistoDebug(ShaderCollector* _shCol);

	void update(GLint actTexId, GLint lastTexId);
	void updateMat();

	// debugging
	void drawPosCol();
	void drawHistogram();
	void drawActivityQuad();

	void setThres(float _val);
	void setTimeMedian(float _val);
	void setPosColThres(float _val);
	void setHistoValThres(float _val);
	void setIndValThres(float _val);
	void setHistoSmooth(float _val);

	GLint getPositionToColorTex();
	GLint getHistoTex();

	glm::mat4* getTransMat();
	glm::mat4* getInvTransMat();
	glm::mat4* getInvTransFlipHMat();

private:
	Quad* quad;
	Quad* rawQuad;
	Quad* ctrlQuad;
	Quad* centQuad;

	FBO* medFbo;
	FBO* colFbo;
	GLSLTimeMedian* tMed;
	GLSLHistogram* histo;
	FastBlurMem* blur;

	CvKalmanFilter* kCent;
	CvKalmanFilter* kSize;
	CvKalmanFilter* kPos;

	Shaders* stdTex;
	Shaders* stdCol;
	Shaders* diffThres;
	Shaders* posColShdr;
	Shaders* histoTex;

	int lastFrame;
	int procFrame = 0;
	int lastProcFrame = 0;

	int lastUpdtTexId = -1;
	int updtTexId = 0;

	float dstRatio;
	float minSize;

	float timeMedian = 3.2f;
	float thres = 0.05f;
	float posColThres = 0.17f;
	float histoValThres = 0.14f;
	float indValThres = 1.5f;
	float histoSmooth = 0.4f;

	unsigned int width;
	unsigned int height;

	glm::mat4 ctrlQuadMat;
	glm::mat4 invQuadMat;
	glm::mat4 invQuadMatFlipH;

	glm::vec2 actQuadSize;
	glm::vec2 actQuadPos;
	glm::vec2 energCent;

	Median<glm::vec2>* quadSize;
	Median<glm::vec2>* quadPos;
	Median<glm::vec2>* energCentMed;
	Median<glm::vec2>* dstQuadSize;
//	Median<glm::vec2>*			dstQuadPos;
	glm::vec2 dstQuadPos;
};

} /* namespace tav */

#endif /* VIDEOACTIVITYRANGE_H_ */
