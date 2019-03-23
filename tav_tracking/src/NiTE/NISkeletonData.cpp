//
//  NISkeletonData.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 04.10.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "NISkeletonData.h"

namespace tav
{

NISkeletonData::NISkeletonData(nite::UserTracker* _userTracker, bool _useKalman,
		float _maxZDist, float _kalmanSmooth) :
		userTracker(_userTracker), useKalman(_useKalman), maxZDist(_maxZDist), kalmanSmooth(
				_kalmanSmooth)
{
	jointTypes = {
		nite::JOINT_HEAD,
		nite::JOINT_NECK,
		nite::JOINT_LEFT_SHOULDER,
		nite::JOINT_RIGHT_SHOULDER,
		nite::JOINT_LEFT_ELBOW,
		nite::JOINT_RIGHT_ELBOW,
		nite::JOINT_LEFT_HAND,
		nite::JOINT_RIGHT_HAND,
		nite::JOINT_TORSO,
		nite::JOINT_LEFT_HIP,
		nite::JOINT_RIGHT_HIP,
		nite::JOINT_LEFT_KNEE,
		nite::JOINT_RIGHT_KNEE,
		nite::JOINT_LEFT_FOOT,
		nite::JOINT_RIGHT_FOOT
	};

	skelActCorFact = 1.3f;
}

//----------------------------------------------------------------

NISkeletonData::~NISkeletonData()
{
	joints.resize(0);
}

//----------------------------------------------------------------

void NISkeletonData::update(nite::UserTrackerFrameRef* _userTrackerFrame)
{
	const nite::Array<nite::UserData>& users = _userTrackerFrame->getUsers();

	float nearest = 100000.f;
	for (auto userId = 0; userId < users.getSize(); ++userId)
	{
		const nite::UserData& user = users[userId];

		// update limbs
		if ((unsigned int) joints.size() < (unsigned int) (userId + 1))
		{
			joints.push_back(new std::vector<NISkeletonPoint>(jointTypes.size()));

			for (size_t i = 0; i < jointTypes.size(); i++)
				joints[userId]->at(i).setKalmanSmootFact(kalmanSmooth);

			skeletonActivity.push_back(0.f);
		}

		short jointInd = 0;
		float actSum = 0.f;

		std::vector<nite::JointType>::iterator it;
		for (it = jointTypes.begin(); it != jointTypes.end(); ++it)
		{
			updtPoint(user.getSkeleton().getJoint((*it)).getPosition(),
					joints[userId]->at(jointInd), userId);

			actSum += joints[userId]->at(jointInd).getAvgActivity();

			jointInd++;
		}

		float usrVisible = float( users[userId].isVisible() );

		skeletonActivity[userId] = fmin(
				(actSum / jointTypes.size() + skeletonActivity[userId] * 3.f)
				/ 4.f * usrVisible, 1.f);

		if (user.getCenterOfMass().z < nearest)
		{
			nearest = user.getCenterOfMass().z;
			nearestId = userId;
		}

		// update center of mass, result is in realworldcoordinates
		if ((unsigned int) centerOfMass.size() < (unsigned int) (userId + 1))
			centerOfMass.push_back(glm::vec3(0.f));
		else
			centerOfMass[userId] = glm::vec3(user.getCenterOfMass().x,
					user.getCenterOfMass().y, user.getCenterOfMass().z);

		// std::cout << joints.size()  << std::endl;
	}
}

//----------------------------------------------------------------

void NISkeletonData::updtPoint(const nite::Point3f& point, NISkeletonPoint& dst,
		short userId)
{
	// convert from real world coordinates to normalize depth coordinates (-1.f - 1.f)
	if (std::isfinite(point.x) && std::isfinite(point.y)
			&& std::isfinite(point.z))
	{
		// erhöhe den ptr, muss im verlauf dieses loop gleich bleiben
		// da werte gelesen und wieder an diesselbe Stelle geschrieben
		// werden müssen
		dst.incPosPtr();

		userTracker->convertJointCoordinatesToDepth(point.x, point.y, point.z,
				dst.getPosXRef(), dst.getPosYRef());

		if (std::isfinite(dst.getPosX()) && std::isfinite(dst.getPosY())
				&& std::isfinite(point.z))
		{
			dst.setPosX(
					dst.getPosX() / static_cast<float>(diWidth) * 2.f - 1.0f);
			dst.setPosY(
					1.f - (dst.getPosY() / static_cast<float>(diHeight) * 2.f));
			dst.setPosZ(point.z / maxZDist); // normalize depth, aprox 4m depth

			// apply kalman filter
			if (useKalman)
			{
				dst.predict();
				dst.kfUpdateFromPos();
			}

			dst.calcDir(3.f);
			dst.calcAvgActivity(3.f);

		}
		else
		{
			// wenn nichts gefunden setze den ptr wieder zurück
			dst.decPosPtr();
		}
	}
}

//----------------------------------------------------------------

float NISkeletonData::getAvgAct(short userId)
{
	float out = 0.f;

	if ((size_t) userId < skeletonActivity.size())
		out = skeletonActivity[userId];
	return out;
}

//----------------------------------------------------------------

glm::vec3 NISkeletonData::getCenter(short userId)
{
	if (centerOfMass.size() > userId)
		return centerOfMass[userId];
	else
	{
		std::cerr << "NISkeletonData::getCenter Error: invalid userId, requested " << userId << " but only " << centerOfMass.size() << " users are available "<< std::endl;
		return glm::vec3(0.f);
	}
}

//----------------------------------------------------------------

glm::vec3 NISkeletonData::getJoint(short userId, nite::JointType type)
{
	glm::vec3 out = glm::vec3(0.f, 0.f, 0.f);
	std::vector<nite::JointType>::iterator it = std::find(jointTypes.begin(), jointTypes.end(), type);

	if (it == jointTypes.end())
		std::cerr << "NISkeletonData::getJoint Error: joint not found " << std::endl;

	if (userId < (short) joints.size())
	{
	//	std::cout << glm::to_string(joints[userId]->at( (short)(it - jointTypes.begin()) ).getPos()) << std::endl;
		out = joints[userId]->at((short) (it - jointTypes.begin())).getPos();
	}
	return out;
}

//----------------------------------------------------------------

glm::vec3 NISkeletonData::getJointDir(short userId, nite::JointType type)
{
	glm::vec3 out = glm::vec3(0.f, 0.f, 0.f);
	std::vector<nite::JointType>::iterator it = std::find(jointTypes.begin(),
			jointTypes.end(), type);
	if (userId < (short) joints.size())
		out =
				joints[userId]->at((short) (it - jointTypes.begin())).getDirNorm();
	return out;
}

//----------------------------------------------------------------

short NISkeletonData::getMostActiveUser()
{
	short uInd = 0;
	float maxAct = 0.f;
	for (size_t i = 0; i < skeletonActivity.size(); i++)
	{
		if (skeletonActivity[i] > maxAct)
		{
			uInd = i;
			maxAct = skeletonActivity[i];
		}
	}

	return uInd;
}

//----------------------------------------------------------------

short NISkeletonData::getNearestUser()
{
	return nearestId;
}

//----------------------------------------------------------------

short NISkeletonData::getNrUsers()
{
	return joints.size();
}

//----------------------------------------------------------------

void NISkeletonData::setUserTracker(nite::UserTracker* _userTracker)
{
	userTracker = _userTracker;
}

//----------------------------------------------------------------

void NISkeletonData::setDimension(int _width, int _height)
{
	diWidth = static_cast<float>(_width);
	diHeight = static_cast<float>(_height);
}

}
