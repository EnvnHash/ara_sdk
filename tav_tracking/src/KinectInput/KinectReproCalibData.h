//
//  KinectReproCalibData.h
//  tav_tracking
//
//  Created by Sven Hahne on 19/11/15.
//  Copyright © 2015 Sven Hahne. All rights reserved..
//

#ifndef KinectReproCalibData_h
#define KinectReproCalibData_h

#pragma once

#include "headers/opencv_headers.h"
#include "glm/glm.hpp"

namespace tav
{
class KinectReproCalibData
{
public:
	enum beamerModel
	{
		ZOOM = 0, WIDE = 1, ULTRA_WIDE = 2
	};

	KinectReproCalibData()
	{
		calibWinCorners.push_back(glm::vec2(-1.f, -1.f)); // left lower
		calibWinCorners.push_back(glm::vec2(1.f, -1.f)); // right lower
		calibWinCorners.push_back(glm::vec2(1.f, 1.f)); // right uppper
		calibWinCorners.push_back(glm::vec2(-1.f, 1.f)); // left uppper

		calibWinCenter = glm::vec2(0.f, 0.f);
		calibWinSize = glm::vec2(2.f, 2.f);
	}

	~KinectReproCalibData()
	{
	}

	bool invertMatr;
	bool hFlip = false;

	cv::Size imgSize;    // image size of the camera input
	cv::Size depthImgSize;
	cv::Size boardSize;
	int nrSamples;
	float beamerAspectRatio;
	float beamerFovX;
	float beamerFovY;
	float beamerLookAngle;
	float beamerLowEdgeAngle;
	float beamerThrowRatio;
	float beamerWallDist;

	float rotXOffs = 0.f;

	float threshZDeep = 4000.f;
	float threshZNear = 0.f;
	float kinCropLeft = -1.f;
	float kinCropRight = 1.f;
	float kinCropUp = 1.f;
	float kinCropBottom = -1.f;

	float kinAspectRatio;
	float kinFovX;
	float kinFovY;
	glm::vec2 kinFov;

	glm::vec2 calibWinSize;
	glm::vec2 calibWinCenter;
	std::vector<glm::vec2> calibWinCorners;

	float distBeamerObj;
	float distKinObj;

	glm::mat4 chessRotXY;
	glm::mat4 chessRotZ;
	glm::mat4 chessTrans;

	glm::mat4 camBeamerRotXY;
	glm::mat4 camBeamerRotZ;
	glm::mat4 camBeamerTrans;
	glm::mat4 camBeamerDist;

	glm::vec3 chessNormal;
	glm::vec3 beamerMidToKinect;
	glm::vec3 camBeamerRealOffs;
	glm::vec3 beamerRealMid;
	glm::vec3 chessRealMid;
	glm::vec3 camBeamerRots;
	glm::vec3 rotations;
	glm::vec3 objRotations;

	beamerModel beamModel;

	cv::Mat cameraMatrix;
	cv::Mat distCoeffs;
	cv::Mat camBeamerPerspTrans;
	cv::Mat camBeamerInvPerspTrans;
};
}

#endif /* KinectReproCalibData_h */
