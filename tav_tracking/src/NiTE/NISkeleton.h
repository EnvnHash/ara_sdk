//
//  NISkeleton.h
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
#include <boost/thread/mutex.hpp>

#include <NiTE.h>


//#include <GLUtils/glm_utils.h>
#include <GLUtils/VAO.h>
#include <Shaders/ShaderCollector.h>

#include "NISkeletonData.h"
#include "KinectInput/KinectInputPostProc.h"

#include <GLFW/glfw3.h>

using std::vector;

namespace tav
{

class NISkeleton: public KinectInputPostProc
{
public:
	NISkeleton(openni::Device* _dev, float _zDistScale);
	~NISkeleton();
	void update(openni::VideoStream& depthStream,
			openni::VideoFrameRef* depthFrame);

	void drawDebug(ShaderCollector* shCol, camPar* cp);
    void DrawStatusLabel(nite::UserTracker* pUserTracker, const nite::UserData& user, float* mat);
    void DrawCenterOfMass(nite::UserTracker* pUserTracker, const nite::UserData& user, float* mat);
    void DrawBoundingBox(const nite::UserData& user, float* mat);
    void DrawLimbFromData(short userId, nite::JointType joint1, nite::JointType joint2, int color, float* mat);
    void DrawSkeletonFromData(nite::UserTracker* pUserTracker, const nite::UserData& userData, float* mat);
    void drawShapes();


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
	nite::UserTracker* getUserTracker();
	NISkeletonData* getSkelData();

	void setUpdateImg(bool _set);
	void setUpdateSkel(bool _set);
	int getFrameNr();
	void reset();
	void myNanoSleep(uint32_t ns);

	NISkeletonData* skelData;
	int max_users;
	int resImgWidth;
	int resImgHeight;
	int resImgNrChans;

	nite::UserTracker*  m_pUserTracker;
	nite::UserTrackerFrameRef userTrackerFrame;

private:

	openni::Device* dev;
	nite::UserId m_poseUser;
	uint64_t m_poseTime;
    Shaders* stdCol;

	bool init;
	bool debugInit;
	bool ready;
	bool userTrackerReady = false;
	bool useKalmanFilter;
	bool updateImg = true;
	bool updateShadow = false;
	bool updateSkeleton;
	bool useThreads;
	bool niteDestroyed = false;
    bool g_drawSkeleton;
    bool g_drawCenterOfMass;
    bool g_drawStatusLabel;
    bool g_drawBoundingBox;

	int actUser = 0;
	int colorCount;
	int frameNr;
	int dryFrames = 0;
    int drawPointPtr=0;
    int skelOffPtr=0;

	float skeletonSmooth;
	float skeletonKSmooth;

	float** Colors;

	float** skelColors;
    float*	centerOfMass;
    float*  boundingBox;

	char**  g_userStatusLabels;

	bool* g_visibleUsers;
	nite::SkeletonState* g_skeletonStates;

	uint8_t* res_img;

	VAO*    pointsVao;
    VAO*    lineLoopVao;
    VAO*    skeletonVao;


	boost::mutex updt_mtx;
};

}
