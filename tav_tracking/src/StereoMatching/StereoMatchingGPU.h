//
//  StereoMatchingGPU.h
//  tav_tracking
//
//  Created by Sven Hahne on 7/7/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __tav_tracking__StereoMatchingGPU__
#define __tav_tracking__StereoMatchingGPU__

#pragma once
#include <stdio.h>
#include <cstring>

#include "GeoPrimitives/Quad.h"
#include "GLUtils/PingPongFbo.h"
#include <KinectInput/KinectInput.h>
#include "StereoMatching/Aggregation.h"

namespace tav
{
class StereoMatchingGPU
{
public:
	StereoMatchingGPU(KinectInput* _kin, ShaderCollector* _shCol);
	~StereoMatchingGPU();
	void compute();
	void init();
	void update();
	void renderRectified();
	void renderCost();
	void renderWinSize();
	void testR32();

private:
	void costInitialization();
	void costAggregation();
	/*
	 void scanlineOptimization();
	 void outlierElimination();
	 void regionVoting();
	 void properInterpolation();
	 void discontinuityAdjustment();
	 void subpixelEnhancement();
	 
	 cv::Mat cost2disparity(int imageNo);
	 */

	Aggregation* aggregation;
	KinectInput* kin;
	ShaderCollector* shCol;
	Shaders* texShader;
	Shaders* adShader;
	Shaders* adAggrShader;
	Shaders* adAggrNormShader;

	Quad** debugQuads;
	Quad* fullQuad;

	PingPongFbo** costMaps;
	PingPongFbo* windowSizePP;
	FBO* aggrTempFBO;
	FBO* aggrNormTempFBO;

	unsigned int nrDevices;
	cv::Mat cameraMatrix[2];
	cv::Mat distCoeffs[2];
	cv::Mat R, T, R1, R2, P1, P2, Q;
	cv::Rect validRoi[2];
	cv::Mat rmap[2][2];
	cv::Mat* img;
	cv::Mat* rimg;
	cv::Mat* testImgFile;

	TextureManager** imgTex;

	int dMin;
	int dMax;
	int nrCostFbos;
	int nrAdFboAttachments;
	cv::Mat images[2];
	cv::Size censusWin;
	cv::Size imageSize;
	cv::Size halfImageSize;
	float defaultBorderCost;
	float lambdaAD;
	float lambdaCensus;
	std::string savePath;
	unsigned int aggregatingIterations;
	unsigned int colorThreshold1;
	unsigned int colorThreshold2;
	unsigned int maxLength1;
	unsigned int maxLength2;
	unsigned int colorDifference;
	float pi1;
	float pi2;
	unsigned int dispTolerance;
	unsigned int votingThreshold;
	float votingRatioThreshold;
	unsigned int maxSearchDepth;
	unsigned int blurKernelSize;
	unsigned int cannyThreshold1;
	unsigned int cannyThreshold2;
	uint cannyKernelSize;
	bool validParams, dispComputed;

	bool bInit;
	bool bValid;
	bool useTestFile;
	bool bAggrInit;

	cv::Size imgSize;
	cv::Mat disparityMap, floatDisparityMap;
	//        ADCensusCV *adCensus;
	//        Aggregation *aggregation;
	//        DisparityRefinement *dispRef;

	unsigned int* frameNr;
};
}

#endif /* defined(__tav_tracking__StereoMatchingGPU__) */
