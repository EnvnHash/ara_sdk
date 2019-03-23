//
//  Aggregation.h
//  tav_tracking
//
//  Created by Sven Hahne on 9/7/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __tav_tracking__Aggregation__
#define __tav_tracking__Aggregation__

#include <stdio.h>
#include <vector>

#include "headers/opencv_headers.h"
#include "GLUtils/FBO.h"
#include "GLUtils/TextureManager.h"
#include "Shaders/ShaderCollector.h"

namespace tav
{
class Aggregation
{
public:
	Aggregation(TextureManager** _inTex, cv::Size* _imageSize,
			ShaderCollector* _shCol, int _colorThreshold1, int _colorThreshold2,
			int _maxLength1, int _maxLength2);
	~Aggregation();
	void getLimits(std::vector<cv::Mat> &upLimits,
			std::vector<cv::Mat> &downLimits, std::vector<cv::Mat> &leftLimits,
			std::vector<cv::Mat> &rightLimits) const;
	void computeLimits();
	void saveToIntDisparity(GLuint texID, cv::Size imgSize,
			std::string filename, bool stretch = true);
	void saveToFloatDisparity(GLuint texID, cv::Size imgSize,
			std::string filename, bool stretch = true);

	FBO** upLimits;
	FBO** downLimits;
	FBO** leftLimits;
	FBO** rightLimits;

private:
	void computeLimit(FBO* _limFbo, int directionH, int directionW,
			int imageNo);

	ShaderCollector* shCol;
	TextureManager** inTex;
	cv::Size* imageSize;
	int colorThreshold1;
	int colorThreshold2;
	int maxLength1;
	int maxLength2;

	Shaders* compLimitsShdr;
	Quad* fullQuad;

	cv::Mat debugMat;
};
}

#endif /* defined(__tav_tracking__Aggregation__) */
