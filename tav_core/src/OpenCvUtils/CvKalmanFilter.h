//
//  CvCvKalmanFilter.h
//  BlobTracking
//
//  Created by Sven Hahne on 19.06.14.
//  Copyright (c) 2014 _Sven Hahne. All rights reserved..
//

#ifndef __BlobTracking__CvCvKalmanFilter__
#define __BlobTracking__CvCvKalmanFilter__

#pragma once 

#include <iostream>
#include "headers/opencv_headers.h"

class CvKalmanFilter
{
public:
	CvKalmanFilter(int _dynam_params, int _measure_params, float smoothFact = 0.001f);
	~CvKalmanFilter();

#ifdef HAVE_OPENCV
	void initPos(float x, float y);
	void initPos(float x, float y, float z);
	void predict();
	cv::Mat getPrediction();
	float getPrediction(int ind);
	void update(float x, float y);
	void update(float x, float y, float z);
	float get(int ind);
	void setSmoothFact(float fact);

private:
	bool init;
	bool addMeasureNoise;

	cv::KalmanFilter *kf;
	cv::Mat_<float> state;
	cv::Mat processNoise;
	cv::Mat_<float> measurement;
	cv::Mat measurement_noise;
	cv::Mat prediction;
	cv::Mat estimated;
	cv::Mat actEstim;
	bool m_bMeasurement;
#endif

};

#endif /* defined(__BlobTracking__CvCvKalmanFilter__) */
