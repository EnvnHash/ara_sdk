//
//  NIHandTracker.h
//  tav
//
//  Created by Sven Hahne on 23.04.14.
//
//

#if (defined _WIN32)
#define PRIu64 "llu"
#else
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif

#pragma once

#include <iostream>
#include <stdio.h>
#include <cstring>
#include <vector>
#include <GLFW/glfw3.h>
#include <boost/thread/mutex.hpp>
#include <NiTE.h>

#include <GLUtils/glm_utils.h>
#include "KinectInput/KinectInputPostProc.h"

using std::vector;

namespace tav
{

class NIHandTracker: public KinectInputPostProc
{
public:
	NIHandTracker(openni::Device* _dev, float _zDistScale);
	~NIHandTracker();
	void update(openni::VideoStream& depthStream,
			openni::VideoFrameRef* depthFrame);
	void updateUserState(const nite::UserData& user, uint64_t ts);
	void Finalize();
	uint8_t* getResImg();
	void onKey(int key, int scancode, int action, int mods);
	bool isInited();
	bool isReady();
	float getAvgAct(short userId);
	glm::vec3 getJoint(short userId, nite::JointType type);
	glm::vec3 getJointDir(short userId, nite::JointType type);
	short getMostActiveUser();
	short getNearestUser();
	const nite::UserMap& getUserMap();
	nite::UserTrackerFrameRef* getUserTrackerFrame();
	void setUpdateImg(bool _set);
	void setUpdateSkel(bool _set);
	int getFrameNr();
	void reset();

	int max_users;
	int resImgWidth;
	int resImgHeight;
	int resImgNrChans;

	nite::HandTracker* m_pUserTracker;
	nite::UserTrackerFrameRef userTrackerFrame;

private:

	openni::Device* dev;
	nite::UserId m_poseUser;
	uint64_t m_poseTime;

	bool init;
	bool ready;
	bool useKalmanFilter;
	bool updateImg = false;
	bool updateShadow = false;
	bool updateSkeleton = true;
	bool useThreads;
	bool niteDestroyed = false;

	int actUser = 0;
	int colorCount;
	int frameNr;
	int dryFrames = 0;

	float skeletonSmooth;
	float skeletonKSmooth;

	float Colors[4][3];

	bool* g_visibleUsers;
	nite::SkeletonState* g_skeletonStates;

	uint8_t* res_img;

	boost::mutex updt_mtx;
};

}
