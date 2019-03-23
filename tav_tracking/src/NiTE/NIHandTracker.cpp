//
//  NIHandTracker.cpp
//  tav
//
//  Created by Sven Hahne on 23.04.14.
//
//

#include "NIHandTracker.h"

namespace tav
{

NIHandTracker::NIHandTracker(openni::Device* _dev, float _zDistScale) :
		KinectInputPostProc(), colorCount(4), max_users(10), frameNr(-1), dev(
				_dev), dryFrames(0)
{
	useKalmanFilter = true;

//    handSmooth = 0.25f;     // glättungsfactor der NiTE Lib
//    handKSmooth = 0.01f;    // fuer den kalman filter, je niedriger desto mehr glättung

	Colors[0][0] = 0.6f;
	Colors[0][1] = 0.f;
	Colors[0][2] = 0.f;
	Colors[1][0] = 0.f;
	Colors[1][1] = 0.6f;
	Colors[1][2] = 0.f;
	Colors[2][0] = 0.f;
	Colors[2][1] = 0.f;
	Colors[2][2] = 0.6f;
	Colors[3][0] = 0.f;
	Colors[3][1] = 0.6f;
	Colors[3][2] = 0.6f;

//    m_pHandTracker = new nite::UserTracker;
	g_visibleUsers = new bool[max_users];
	//g_skeletonStates = new nite::SkeletonState[max_users];
	//skelData = new NIHandTrackerData(m_pUserTracker, useKalmanFilter, _zDistScale, skeletonKSmooth);

	for (int i = 0; i < max_users; i++)
	{
		g_visibleUsers[i] = false;
		g_skeletonStates[i] = nite::SKELETON_NONE;
	}

	resImgWidth = 0;
	resImgHeight = 0;
	resImgNrChans = 4;

	// NiTE init
	nite::NiTE::initialize();

	if (m_pUserTracker->create(dev) != nite::STATUS_OK)
	{
		std::cout << "NIHandTracker: failed to open Kinect device" << std::endl;
		exit(1);
	}
	else
	{
		ready = true;
	}

	init = false;
}

//------------------------------------------------------------------------------

void NIHandTracker::update(openni::VideoStream& _stream,
		openni::VideoFrameRef* _frame)
{
	if (!init)
	{
		// make image
		openni::VideoMode videoMode = _frame->getVideoMode();
		resImgWidth = videoMode.getResolutionX();
		resImgHeight = videoMode.getResolutionY();
		resImgNrChans = 4; // BGRA

		// using rgba since it´s the fastest
		res_img = new uint8_t[resImgWidth * resImgHeight * resImgNrChans];
		memset(res_img, 0,
				resImgWidth * resImgHeight * resImgNrChans * sizeof(uint8_t));

		//skelData->setDimension(resImgWidth, resImgHeight);

		init = true;
	}

	if (ready && _stream.isValid() && dryFrames > 10 && doUpdate)
	{
		try
		{
			updt_mtx.lock();
			/*
			 
			 nite::Status rc = m_pUserTracker->readFrame(&userTrackerFrame);
			 
			 if (rc != nite::STATUS_OK)
			 {
			 printf("GetNextData failed\n");
			 return;
			 }
			 
			 m_pUserTracker->setSkeletonSmoothingFactor(skeletonSmooth);
			 
			 *_frame = userTrackerFrame.getDepthFrame();
			 const nite::UserMap& userLabels = userTrackerFrame.getUserMap();
			 
			 if (updateImg && _frame->isValid())
			 {
			 float factor = 1;
			 const nite::UserId* pLabels = userLabels.getPixels();
			 const openni::DepthPixel* pDepthRow = (const openni::DepthPixel*) _frame->getData();
			 uint8_t* pTexRow = res_img + _frame->getCropOriginY() * resImgWidth;
			 int rowSize = _frame->getStrideInBytes() / sizeof(openni::DepthPixel);
			 int colRowSize = resImgWidth * resImgNrChans;
			 
			 for (int y=0; y < _frame->getHeight(); ++y)
			 {
			 const openni::DepthPixel* pDepth = pDepthRow;   // read first depth pixel at actual row
			 uint8_t* pTex = pTexRow + _frame->getCropOriginX(); // get the pointer to the first pixel in the actual row
			 
			 for (int x=0; x < _frame->getWidth(); ++x, ++pDepth, ++pLabels)
			 {
			 if (*pLabels == 0 || *pDepth == 0) factor = 0; else factor = 255.f;
			 *(pTex++) = Colors[*pLabels % colorCount][0] * factor;     // b
			 *(pTex++) = Colors[*pLabels % colorCount][1] * factor;     // g
			 *(pTex++) = Colors[*pLabels % colorCount][2] * factor;     // r
			 *(pTex++) = factor;        // a
			 }
			 
			 pDepthRow += rowSize;   // count up one row
			 pTexRow += resImgWidth * resImgNrChans;
			 }
			 }
			 
			 frameNr++;
			 
			 // process User Tracking
			 if (updateSkeleton)
			 {
			 const nite::Array<nite::UserData>& users = userTrackerFrame.getUsers();
			 for (int i=0; i<users.getSize(); ++i)
			 {
			 const nite::UserData& user = users[i];
			 updateUserState(user, userTrackerFrame.getTimestamp());
			 
			 if (user.isNew())
			 {
			 m_pUserTracker->startSkeletonTracking(user.getId());
			 //m_pUserTracker->startPoseDetection(user.getId(), nite::POSE_CROSSED_HANDS);
			 }
			 }
			 }
			 
			 skelData->update(&userTrackerFrame);
			 
			 //printf("update NiTE unlocking mutex\n");
			 */

			updt_mtx.unlock();
		} catch (std::exception& e)
		{
		}
	}

	dryFrames++;
}

//------------------------------------------------------------------------------

void NIHandTracker::updateUserState(const nite::UserData& user, uint64_t ts)
{
	if (user.isNew())
	{
		std::cout << "New" << std::endl;
		actUser = (short) (user.getId() - 1);
	}
	else if (user.isVisible() && !g_visibleUsers[user.getId()])
		printf("[%08" PRIu64 "] User #%d:\tVisible\n", ts, user.getId());
	else if (!user.isVisible() && g_visibleUsers[user.getId()])
		printf("[%08" PRIu64 "] User #%d:\tOut of SceneNode\n", ts,
				user.getId());
	else if (user.isLost())
	{
		std::cout << ("Lost");
	}
	g_visibleUsers[user.getId()] = user.isVisible();

}

//------------------------------------------------------------------------------

void NIHandTracker::onKey(int key, int scancode, int action, int mods)
{
}

//------------------------------------------------------------------------------

void NIHandTracker::Finalize()
{
	nite::NiTE::shutdown();
	//delete m_pUserTracker;
}

//------------------------------------------------------------------------------

void NIHandTracker::reset()
{
	updt_mtx.lock();

	ready = false;

	if (!niteDestroyed)
	{
		userTrackerFrame.release();
		m_pUserTracker->destroy();
		delete m_pUserTracker;
		nite::NiTE::shutdown();

		niteDestroyed = true;
	}

	// start new init
	//  m_pUserTracker = new nite::UserTracker;
	nite::NiTE::initialize();

	if (m_pUserTracker->create(dev) != nite::STATUS_OK)
		std::cout << "NIHandTracker: failed to open Kinect device" << std::endl;

	// refresh references
	//skelData->setUserTracker(m_pUserTracker);

	ready = true;
	niteDestroyed = false;

	updt_mtx.unlock();
}

//------------------------------------------------------------------------------

uint8_t* NIHandTracker::getResImg()
{
	return res_img;
}

//------------------------------------------------------------------------------

float NIHandTracker::getAvgAct(short userId)
{
	//return skelData->getAvgAct(userId);
	return 0.f;
}

//------------------------------------------------------------------------------

glm::vec3 NIHandTracker::getJoint(short userId, nite::JointType type)
{
	//  return skelData->getJoint(userId, type);
	return glm::vec3(0.f);
}

//------------------------------------------------------------------------------

glm::vec3 NIHandTracker::getJointDir(short userId, nite::JointType type)
{
	//return skelData->getJointDir(userId, type);
	return glm::vec3(0.f);

}

//------------------------------------------------------------------------------

short NIHandTracker::getMostActiveUser()
{
	// return skelData->getMostActiveUser();
	return 0;
}

//------------------------------------------------------------------------------

short NIHandTracker::getNearestUser()
{
	//return skelData->getNearestUser();
	return 0;
}

//------------------------------------------------------------------------------

const nite::UserMap& NIHandTracker::getUserMap()
{
	return userTrackerFrame.getUserMap();
}

//------------------------------------------------------------------------------

nite::UserTrackerFrameRef* NIHandTracker::getUserTrackerFrame()
{
	nite::UserTrackerFrameRef* out = 0;

	if (userTrackerFrame.isValid())
		out = &userTrackerFrame;

	return out;
}

//------------------------------------------------------------------------------

void NIHandTracker::setUpdateImg(bool _set)
{
	updateImg = _set;
}

//------------------------------------------------------------------------------

void NIHandTracker::setUpdateSkel(bool _set)
{
	updateSkeleton = _set;
}

//------------------------------------------------------------------------------

int NIHandTracker::getFrameNr()
{
	return frameNr;
}

//------------------------------------------------------------------------------

NIHandTracker::~NIHandTracker()
{
	Finalize();
	delete res_img;
}

//------------------------------------------------------------------------------

bool NIHandTracker::isInited()
{
	return init;
}

//------------------------------------------------------------------------------

bool NIHandTracker::isReady()
{
	return ready;
}

}
