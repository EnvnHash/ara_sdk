//
//  NISkeletonPoint.h
//  tav_gl4
//
//  Created by Sven Hahne on 04.10.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#ifndef __tav_gl4__NISkeletonPoint__
#define __tav_gl4__NISkeletonPoint__

#include <stdio.h>
#include <string>
#include <cstdlib>

#include <NiTE.h>

#include <GLUtils/glm_utils.h>
#include <OpenCvUtils/CvKalmanFilter.h>
#include <math_utils.h>

namespace tav
{

class NISkeletonPoint
{
public:
	NISkeletonPoint(float _kalmanSmoothFact = 0.001f);
	~NISkeletonPoint();
	void calcDir(float smooth = 0.f);
	void calcAvgActivity(float smooth = 0.f);
	void predict();
	void kfUpdateFromPos();

	void incPosPtr();
	void decPosPtr();

	glm::vec3 getPos();
	glm::vec3* getPosRef();

	glm::vec3 getDir();
	glm::vec3 getDirNorm();

	float getAvgActivity();

	float getPosX();
	float getPosY();
	float getPosZ();

	float* getPosXRef();
	float* getPosYRef();
	float* getPosZRef();

	std::string getPosString();

	void setPosX(float val, float smooth = 0.f);
	void setPosY(float val, float smooth = 0.f);
	void setPosZ(float val, float smooth = 0.f);

	void setKalmanSmootFact(float fact);
	float getRandF(float min, float max);

private:
	CvKalmanFilter* kf;
	glm::vec3* pos;
	short posPtr = 0;
	short posMem;

	glm::vec3* dir;
	short dirPtr = 0;
	short dirMem;

	float avgMovement = 0.f;
	float avgActivity = 0.f;
	float avgActivityScale;

	float kalmanSmoothFact;
};

}

#endif /* defined(__tav_gl4__NISkeletonPoint__) */
