//
//  NISkeletonPoint.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 04.10.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "NISkeletonPoint.h"

namespace tav
{

NISkeletonPoint::NISkeletonPoint(float _kalmanSmoothFact) :
		kalmanSmoothFact(_kalmanSmoothFact)
{
	posMem = 20;
	pos = new glm::vec3[posMem];
	for (auto i = 0; i < posMem; i++)
		pos[i] = glm::vec3(0.f);

	dirMem = 4;
	dir = new glm::vec3[dirMem];
	for (auto i = 0; i < dirMem; i++)
		dir[i] = glm::vec3(0.f);

	kf = new CvKalmanFilter(6, 3, kalmanSmoothFact);
	kf->initPos(getRandF(-1.f, 1.f), getRandF(-1.f, 1.f), getRandF(-1.f, 1.f));

	avgActivityScale = 35.f;

	posPtr = 0;
}

//------------------------------------------------------

void NISkeletonPoint::calcDir(float smooth)
{
	dirPtr = (dirPtr + 1) % dirMem;

	if (smooth > 0.f)
	{
		dir[dirPtr] = (dir[(dirPtr - 1 + dirMem) % dirMem]
				+ (pos[posPtr] - pos[(posPtr - 1 + posMem) % posMem]) * smooth)
				/ (smooth + 1.f);
	}
	else
	{
		dir[dirPtr] = pos[posPtr] - pos[(posPtr - 1 + posMem) % posMem];
	}
}

//------------------------------------------------------

void NISkeletonPoint::calcAvgActivity(float smooth)
{
	float sum = 0.f;
	for (auto i = 1; i < posMem; i++)
		sum += fabs(glm::length(pos[i] - pos[i - 1]));

	sum /= static_cast<float>(posMem - 1);
	sum *= avgActivityScale;

	if (smooth > 0.f)
	{
		avgActivity = (sum + avgMovement * smooth) / (smooth + 1.f);
	}
	else
	{
		avgActivity = sum;
	}
}

//------------------------------------------------------

void NISkeletonPoint::predict()
{
	kf->predict();
}

//------------------------------------------------------

void NISkeletonPoint::kfUpdateFromPos()
{
	kf->update(pos[posPtr].x, pos[posPtr].y, pos[posPtr].z);
	pos[posPtr].x = kf->get(0);
	pos[posPtr].y = kf->get(1);
	pos[posPtr].z = kf->get(2);
}

//------------------------------------------------------

void NISkeletonPoint::incPosPtr()
{
	posPtr = (posPtr + 1) % posMem;
}

//------------------------------------------------------

void NISkeletonPoint::decPosPtr()
{
	posPtr = (posPtr - 1 + posMem) % posMem;
}

//------------------------------------------------------

NISkeletonPoint::~NISkeletonPoint()
{
	delete kf;
}

//------------------------------------------------------

glm::vec3 NISkeletonPoint::getPos()
{
	return pos[posPtr];
}

//------------------------------------------------------

glm::vec3* NISkeletonPoint::getPosRef()
{
	return &pos[posPtr];
}

//------------------------------------------------------

glm::vec3 NISkeletonPoint::getDir()
{
	return dir[dirPtr];
}

//------------------------------------------------------

glm::vec3 NISkeletonPoint::getDirNorm()
{
	return glm::normalize(dir[dirPtr]);
}

//------------------------------------------------------

float NISkeletonPoint::getAvgActivity()
{
	return avgActivity;
}

//------------------------------------------------------

float NISkeletonPoint::getPosX()
{
	return pos[posPtr].x;
}

//------------------------------------------------------

float NISkeletonPoint::getPosY()
{
	return pos[posPtr].y;
}

//------------------------------------------------------

float NISkeletonPoint::getPosZ()
{
	return pos[posPtr].z;
}

//------------------------------------------------------

float* NISkeletonPoint::getPosXRef()
{
	return &pos[posPtr].x;
}

//------------------------------------------------------

float* NISkeletonPoint::getPosYRef()
{
	return &pos[posPtr].y;
}

//------------------------------------------------------

float* NISkeletonPoint::getPosZRef()
{
	return &pos[posPtr].z;
}

//------------------------------------------------------

std::string NISkeletonPoint::getPosString()
{
	return glm::to_string(pos[posPtr]);
}

//------------------------------------------------------

void NISkeletonPoint::setPosX(float val, float smooth)
{
	if (smooth > 0.f)
	{
		pos[posPtr].x = (val + pos[(posPtr - 1 + posMem) % posMem].x * smooth)
				/ (smooth + 1.f);
	}
	else
	{
		pos[posPtr].x = val;
	}
}

//------------------------------------------------------

void NISkeletonPoint::setPosY(float val, float smooth)
{
	if (smooth > 0.f)
	{
		pos[posPtr].y = (val + pos[(posPtr - 1 + posMem) % posMem].x * smooth)
				/ (smooth + 1.f);
	}
	else
	{
		pos[posPtr].y = val;
	}
}

//------------------------------------------------------

void NISkeletonPoint::setPosZ(float val, float smooth)
{
	if (smooth > 0.f)
	{
		pos[posPtr].z = (val + pos[(posPtr - 1 + posMem) % posMem].x * smooth)
				/ (smooth + 1.f);
	}
	else
	{
		pos[posPtr].z = val;
	}
}

//------------------------------------------------------

void NISkeletonPoint::setKalmanSmootFact(float fact)
{
	kf->setSmoothFact(fact);
}

//------------------------------------------------------

float NISkeletonPoint::getRandF(float min, float max)
{
	float outVal = 0.0f;
	outVal = rand() % 100000;
	outVal *= 0.00001f;

	outVal *= max - min;
	outVal += min;

	return outVal;
}

}
