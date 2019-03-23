//
//  TrackObj.cpp
//  BlobTracking
//
//  Created by Sven Hahne on 16.06.14.
//  Copyright (c) 2014 __Zeitkunst_. All rights reserved.
//

#include "TrackObj.h"


TrackObj::TrackObj() : drawKalman(false), bAccumVel(false), maxLinePoints(400),
	velMed(3.f), velCorrectFact(5.0f), minSpeed(0.00001f), speedThres(0.004f),
	inUse(false), resetMed(true), lastUpdtTime(0.0)
{
	index_str = std::to_string(index);

	actPos = glm::vec2(0.f, 0.f);
//        actPos = glm::vec2(getRandF(2.0f, 1.0f), getRandF(2.0f, 1.0f));
	actRawPos = actPos;
	actVel = glm::vec2(0.0f, 0.0f);
	medVel = glm::vec2(0.0f, 0.0f);
	actAccel = glm::vec2(0.0f, 0.0f);
	actRot = 0.0f;
	memPosSize = 2;
	memPosPtr = 0;

	memPos = new glm::vec2[memPosSize];
	for (int i = 0; i < memPosSize; i++)
		memPos[i] = glm::vec2(0.0f, 0.0f);

	kf = new CvKalmanFilter(4, 2, 0.3f);
	kf->initPos(actPos.x, actPos.y);

	state = OFF;
	lastState = OFF;

}

//-------------------------------------------------------------------------------------------

TrackObj::~TrackObj()
{
//        delete posBuf;
//        delete kalBuf;
}

//-------------------------------------------------------------------------------------------

void TrackObj::update(float newX, float newY, float dt)
{
	// apply kalman filtering
	kf->update(newX, newY);

	// save old State
	memPos[memPosPtr] = actPos;
	memPosPtr = (memPosPtr + 1) % memPosSize;

	actPos.x = kf->get(0);
	actPos.y = kf->get(1);
	actRawPos.x = newX;
	actRawPos.y = newY;

	//actRot = rot;

	// calc velocity
	if (memPos[(memPosPtr - 1 + memPosSize) % memPosSize].x != 0.f
			&& memPos[(memPosPtr - 1 + memPosSize) % memPosSize].y != 0.f)
		actVel = (actPos - memPos[(memPosPtr - 1 + memPosSize) % memPosSize])
				* velCorrectFact;

//        if(resetMed)
//        {
//            resetMed = false;
//            medVel = actVel;
//        } else {
	if (medVel.x == 0.f && medVel.y == 0.f)
		medVel = actVel;
	else
		medVel = (medVel * velMed + actVel) / (velMed + 1.f);
//        }

	if (bAccumVel)
		velAccu += actVel;

	actAccel = actVel / dt;

	// debug drawing lines
	if (drawKalman)
	{
		/*
		 if ( posBuf->size() >= maxLinePoints ) {
		 posBuf->erase_vertex(0); posBuf->erase_color(0);
		 }
		 posBuf->push_back_vertex(newX, newY, -0.5f);
		 posBuf->push_back_color(1.0f, 0.0f, 0.0f, 1.0f);

		 if ( kalBuf->size() >= maxLinePoints ) {
		 kalBuf->erase_vertex(0); kalBuf->erase_color(0);
		 }
		 kalBuf->push_back_vertex(kf->get(0), kf->get(1), -0.5f);
		 kalBuf->push_back_color(0.0f, 1.0f, 0.0f, 1.0f);
		 */
	}
}

//-------------------------------------------------------------------------------------------

float TrackObj::getDist(float newX, float newY, float rot)
{
	float adj = 0.0f;
	//glm::vec2 lastPos = memPos[(memPosPtr - 1 + memPosSize) % memPosSize];
	glm::vec2 newPos = glm::vec2(newX, newY);
	glm::vec2 newDir = newPos - actRawPos;

	//glm::vec2 oldDir = actPos - lastPos;

	//float dirDiff = glm::angle(newDir, oldDir);
	float rotDiff = fabs(fabs(actRot - M_PI) - fabs(rot - M_PI));

	// wenn in bewegung und richtung mehr als 180 grad verschieden, mach ungültig
	//    if ( newDir.length() > minSpeed && oldDir.length() > minSpeed && fabs(dirDiff) > PI ) adj = 1000.0f;
	// if ( oldDir.length() < minSpeed && newDir.length() > speedThres ) adj = 1000.0f;

	glm::vec4 pos = glm::vec4(newX - kf->getPrediction(0),
			newY - kf->getPrediction(1),
			(newDir.x - kf->getPrediction(2)) * velCorrectFact,
			(newDir.y - kf->getPrediction(3)) * velCorrectFact);

	return std::sqrt(
			pos.x * pos.x + pos.y * pos.y + pos.z * pos.z + pos.w * pos.w
					+ rotDiff * rotDiff) + adj;
}

//-------------------------------------------------------------------------------------------

void TrackObj::accumVel(bool _val)
{
	if (_val && !bAccumVel)
		velAccu = glm::vec2(0.f);
	bAccumVel = _val;
}

//-------------------------------------------------------------------------------------------

void TrackObj::setPos(float x, float y)
{
	actPos.x = x;
	actPos.y = y;

	if (moveArea.x > x) moveArea.x = x;
	if (moveArea.y > y) moveArea.y = y;

	if (moveArea.z < x) moveArea.z = x;
	if (moveArea.w < y) moveArea.w = y;
}

//-------------------------------------------------------------------------------------------

float TrackObj::getRandF(float min, float max)
{
	float outVal = 0.0f;
	outVal = rand() % 100000;
	outVal *= 0.00001f;

	outVal *= max - min;
	outVal += min;

	return outVal;
}

//-------------------------------------------------------------------------------------------

void TrackObj::setOn()
{
	if (state == OFF)
	{
		onDate = std::chrono::system_clock::now();
		moveArea = glm::vec4(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
				std::numeric_limits<float>::min(), std::numeric_limits<float>::min());

		// create a unique id by date
		uniqueId = (int)std::chrono::duration_cast<std::chrono::milliseconds>(onDate.time_since_epoch()).count();
		uniqueId += (int)((long)this);
	    lastState = state;
	}
	state = ON;
}

//-------------------------------------------------------------------------------------------

void TrackObj::setOff()
{
	offDate = std::chrono::system_clock::now();

	lastState = state;
	state = OFF;
	resetMed = true;
}
