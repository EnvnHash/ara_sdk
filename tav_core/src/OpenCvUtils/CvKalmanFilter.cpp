//
//  CvCvKalmanFilter.cpp
//  BlobTracking
//
//  Created by Sven Hahne on 19.06.14.
//  Copyright (c) 2014 __Zeitkunst_. All rights reserved.
//

#include "pch.h"
#include "OpenCvUtils/CvKalmanFilter.h"

CvKalmanFilter::CvKalmanFilter(int _dynam_params, int _measure_params,
		float smoothFact)
{
#ifdef HAVE_OPENCV
	init = false;
	addMeasureNoise = false;

	int dynam_params = _dynam_params;	// x,y,dx,dy//,width,height
	int measure_params = _measure_params;

	kf = new cv::KalmanFilter(dynam_params, measure_params, 0);
	//cv::Ptr<cv::KalmanFilter> kfa = cv::makePtr<cv::KalmanFilter>;

	state = cv::Mat(dynam_params, 1, CV_32FC1);
	state.setTo(cv::Scalar(0));
	actEstim = cv::Mat(dynam_params, 1, CV_32FC1);
	processNoise = cv::Mat(dynam_params, 1, CV_32FC1); // (w_k)
	measurement = cv::Mat(measure_params, 1, CV_32FC1); // two parameters for x,y  (z_k)
	measurement.setTo(cv::Scalar(0));
	measurement_noise = cv::Mat(measure_params, 1, CV_32FC1); // two parameters for x,y (v_k)

	//kf->transitionMatrix = (cv::Mat_<float>(4, 4) << 1,0,0,0, 0,1,0,0,  0,0,1,0,  0,0,0,1);

	// F matrix data
	// F is transition matrix.  It relates how the states interact
	// For single input fixed velocity the new value
	// depends on the previous value and velocity- hence 1 0 1 0
	// on top line. New velocity is independent of previous
	// value, and only depends on previous velocity- hence 0 1 0 1 on second row
	if (_dynam_params == 4)
	{
		kf->transitionMatrix = (cv::Mat_<float>(4, 4) << 1, 0, 1, 0, // 0, 0,	//x + dx
		0, 1, 0, 1,    // 0, 0,	//y + dy
		0, 0, 1, 0,    // 0, 0,	//dx = dx
		0, 0, 0, 1    // 0, 0,     //dy = dy
				//0, 0, 0, 0, 1, 0,	//width
				//0, 0, 0, 0, 0, 1,	//height
				);
	}
	else if (_dynam_params == 6)
	{
		kf->transitionMatrix =
				(cv::Mat_<float>(6, 6) << 1, 0, 0, 0.5, 0, 0, 0, 1, 0, 0, 0.5, 0, 0, 0, 1, 0, 0, 0.5, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1);

	}
	else
	{
		printf(
				"CvKalmanFilter Error: up to now only 2 or 3 measure parameters supported!\n");
	}

	setIdentity(kf->measurementMatrix, cv::Scalar::all(1)); // (H)
	setIdentity(kf->processNoiseCov, cv::Scalar::all(smoothFact)); // (Q) grad der glättung je höher desto näher dran
	setIdentity(kf->measurementNoiseCov, cv::Scalar::all(1e-1)); // (R)
	setIdentity(kf->errorCovPost, cv::Scalar::all(.1));

	randn(kf->statePost, cv::Scalar::all(0), cv::Scalar::all(0.1));
#endif
}

//------------------------------------------------------------------------------------

CvKalmanFilter::~CvKalmanFilter()
{
//    delete &measurement;
//    delete kf;
}

//------------------------------------------------------------------------------------
#ifdef HAVE_OPENCV

void CvKalmanFilter::initPos(float x, float y)
{
	// Initialize kalman variables (change in x, change in y,...)
	kf->statePre.at<float>(0) = y;	//center x
	kf->statePre.at<float>(1) = x;	//center y
	kf->statePre.at<float>(2) = (float) 0;	 //dx
	kf->statePre.at<float>(3) = (float) 0;	 //dy
}

//------------------------------------------------------------------------------------

void CvKalmanFilter::initPos(float x, float y, float z)
{
	// Initialize kalman variables (change in x, change in y,...)
	kf->statePre.at<float>(0) = x;	//center x
	kf->statePre.at<float>(1) = y;	//center y
	kf->statePre.at<float>(2) = z;	//center z
	kf->statePre.at<float>(3) = (float) 0;	 //dx
	kf->statePre.at<float>(4) = (float) 0;	 //dy
	kf->statePre.at<float>(5) = (float) 0;	 //dz
}

//------------------------------------------------------------------------------------

void CvKalmanFilter::predict()
{
	prediction = kf->predict();
	// std::cout << "predict: " << prediction << std::endl;
}

//------------------------------------------------------------------------------------

cv::Mat CvKalmanFilter::getPrediction()
{
	return prediction;
}

//------------------------------------------------------------------------------------

float CvKalmanFilter::getPrediction(int ind)
{
	return prediction.at<float>(ind);
}

//------------------------------------------------------------------------------------

void CvKalmanFilter::update(float x, float y)
{
	// If we have received the real position recently, then use that position to correct
	measurement(0) = x;
	measurement(1) = y;
//        measurement(2) = x - state.at<float>(0);
//        measurement(3) = y - state.at<float>(1);

	state.at<float>(0) = x;
	state.at<float>(1) = y;

	if (addMeasureNoise)
	{
		// generate measurement noise(z_k)
		randn(measurement_noise, cv::Scalar(0),
				cv::Scalar::all(sqrt(kf->measurementNoiseCov.at<float>(0, 0))));

		// zk = Hk * xk + vk
		measurement = kf->measurementMatrix * state + measurement_noise;
	}

	// adjust Kalman filter state
	estimated = kf->correct(measurement);

	// std::cout << "result " << estimated << std::endl;

	actEstim = cv::Mat_<float>(estimated);

	if (addMeasureNoise)
	{
		// generate process noise(w_k)
		randn(processNoise, cv::Scalar(0),
				cv::Scalar::all(sqrt(kf->processNoiseCov.at<float>(0, 0))));

		// xk = F * xk-1 + B * uk + wk
		state = kf->transitionMatrix * state + processNoise;
	}
}

//------------------------------------------------------------------------------------

void CvKalmanFilter::update(float x, float y, float z)
{
	// If we have received the real position recently, then use that position to correct
	measurement(0) = x;
	measurement(1) = y;
	measurement(2) = z;

	state.at<float>(0) = x;
	state.at<float>(1) = y;
	state.at<float>(2) = z;

	if (addMeasureNoise)
	{
		randn(measurement_noise, cv::Scalar(0),
				cv::Scalar::all(sqrt(kf->measurementNoiseCov.at<float>(0, 0))));
		measurement = kf->measurementMatrix * state + measurement_noise;
	}

	// adjust Kalman filter state
	estimated = kf->correct(measurement);
	actEstim = cv::Mat_<float>(estimated);

	if (addMeasureNoise)
	{
		randn(processNoise, cv::Scalar(0),
				cv::Scalar::all(sqrt(kf->processNoiseCov.at<float>(0, 0))));
		state = kf->transitionMatrix * state + processNoise;
	}
}

//------------------------------------------------------------------------------------

float CvKalmanFilter::get(int ind)
{
	return actEstim.at<float>(ind);
}

//------------------------------------------------------------------------------------

void CvKalmanFilter::setSmoothFact(float fact)
{
	setIdentity(kf->processNoiseCov, cv::Scalar::all(fact)); // (Q) grad der glättung je höher desto näher dran
}

#endif
