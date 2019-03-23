//
//  NISkeletonData.h
//  tav_gl4
//
//  Created by Sven Hahne on 04.10.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#ifndef __tav_gl4__NISkeletonData__
#define __tav_gl4__NISkeletonData__

#include <iostream>
#include <stdio.h>
#include <vector>
#include "NISkeletonPoint.h"

namespace tav
{

class NISkeletonData
{
public:
	NISkeletonData(nite::UserTracker* _userTracker, bool _useKalman = false,
			float _maxZDist = 4000.f, float _kalmanSmooth = 0.0005f);
	~NISkeletonData();

	void update(nite::UserTrackerFrameRef* _userTrackerFrame);
	void updtPoint(const nite::Point3f& point, NISkeletonPoint& dst,
			short userId);

	void setDimension(int _width, int _height);

	float getAvgAct(short userId);
	glm::vec3 getCenter(short userId);
	glm::vec3 getJoint(short userId, nite::JointType type);
	glm::vec3 getJointDir(short userId, nite::JointType type);
	short getMostActiveUser();
	short getNearestUser();
	short getNrUsers();
	void setUserTracker(nite::UserTracker* _userTracker);

private:
	nite::UserTracker* userTracker;
	std::vector<nite::JointType> jointTypes;
	std::vector<glm::vec3> centerOfMass;
	std::vector<std::vector<NISkeletonPoint>*> joints;
	std::vector<float> skeletonActivity;
	float diWidth;
	float diHeight;
	float maxZDist;
	float kalmanSmooth;
	float skelActCorFact;

	bool useKalman;
	short nearestId = -1;
};

}
#endif /* defined(__tav_gl4__NISkeletonData__) */
