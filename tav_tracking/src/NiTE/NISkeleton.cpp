//
//  NISkeleton.cpp
//  tav
//
//  Created by Sven Hahne on 23.04.14.
//
//  Skeleton Tracking muss explizipt mit setUpdateSkel(true) aktiviert werden
//  Das Erstellen der UserMap (bitmap) muss explizit mit setUpdateImage aktiviert werden
//

#include "NiTE/NISkeleton.h"


namespace tav
{

NISkeleton::NISkeleton(openni::Device* _dev, float _zDistScale) :
		KinectInputPostProc(), colorCount(4), max_users(10), frameNr(-1), init(false),
		dev(_dev), useKalmanFilter(true), skeletonKSmooth(0.01f), skeletonSmooth(0.25f),
		debugInit(false), updateSkeleton(true)
{
	// user map colors
	Colors = new float*[max_users];
	for (int i=0;i<max_users;i++)
		Colors[i] = new float[3];

	Colors[0][0] = 0.6f; Colors[0][1] = 0.f; Colors[0][2] = 0.f;
	Colors[1][0] = 0.f; Colors[1][1] = 0.6f; Colors[1][2] = 0.f;
	Colors[2][0] = 0.f; Colors[2][1] = 0.f; Colors[2][2] = 0.6f;
	Colors[3][0] = 0.f;	Colors[3][1] = 0.6f; Colors[3][2] = 0.6f;

	Colors[4][0] = 0.6f; Colors[4][1] = 0.f; Colors[4][2] = 0.f;
	Colors[5][0] = 0.f; Colors[5][1] = 0.6f; Colors[5][2] = 0.f;
	Colors[6][0] = 0.f; Colors[6][1] = 0.f; Colors[6][2] = 0.6f;
	Colors[7][0] = 0.f;	Colors[7][1] = 0.6f; Colors[7][2] = 0.6f;
	Colors[8][0] = 0.6f; Colors[8][1] = 0.f; Colors[8][2] = 0.f;
	Colors[9][0] = 0.f; Colors[9][1] = 0.6f; Colors[9][2] = 0.f;

	nite::NiTE::initialize();
	std::cout << "NISkeleton::init: nite version: " << nite::NiTE::getVersion().major << "." << nite::NiTE::getVersion().minor << std::endl;

	if (_dev)
	{
		m_pUserTracker = new nite::UserTracker;

		if (m_pUserTracker->create(_dev) != nite::STATUS_OK)
		{
			std::cerr << "NISkeleton: failed to create User tracker, probably problem with oni device" << std::endl;

		} else userTrackerReady = true;

	} else 	std::cout << "NISkeleton: failed to open Kinect device" << std::endl;

//    std::this_thread::sleep_for(1s);
	//myNanoSleep(100000000);  // was needed on osx...

	g_visibleUsers = new bool[max_users];
	g_skeletonStates = new nite::SkeletonState[max_users];
	skelData = new NISkeletonData(m_pUserTracker, useKalmanFilter, _zDistScale,
			skeletonKSmooth);

	for (int i = 0; i < max_users; i++)
	{
		g_visibleUsers[i] = false;
		g_skeletonStates[i] = nite::SKELETON_NONE;
	}

	m_pUserTracker->setSkeletonSmoothingFactor(skeletonSmooth);

	resImgWidth = 0;
	resImgHeight = 0;
	resImgNrChans = 4;

	ready = true;

}

//------------------------------------------------------------------------------

void NISkeleton::update(openni::VideoStream& _stream,
		openni::VideoFrameRef* _frame)
{
	if (_frame->isValid() && _stream.isValid())
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
			memset(res_img, 0, resImgWidth * resImgHeight * resImgNrChans * sizeof(uint8_t));

			skelData->setDimension(resImgWidth, resImgHeight);

			init = true;

		}
		else
		{
			updt_mtx.lock();

			nite::Status rc = nite::STATUS_ERROR;

			rc = m_pUserTracker->readFrame(&userTrackerFrame);

			if (rc == nite::STATUS_ERROR)
				std::cerr <<  "m_pUserTracker->readFrame error" << std::endl;

			if (rc == nite::STATUS_OK && userTrackerFrame.isValid())
			{
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

					for (int y = 0; y < _frame->getHeight(); ++y)
					{
						const openni::DepthPixel* pDepth = pDepthRow; // read first depth pixel at actual row
						uint8_t* pTex = pTexRow + _frame->getCropOriginX(); // get the pointer to the first pixel in the actual row

						for (int x = 0; x < _frame->getWidth(); ++x, ++pDepth, ++pLabels)
						{
							if (*pLabels == 0 || *pDepth == 0)
								factor = 0;
							else
								factor = 255.f;

							*(pTex++) = Colors[*pLabels % colorCount][0] * factor;     // b
							*(pTex++) = Colors[*pLabels % colorCount][1] * factor;     // g
							*(pTex++) = Colors[*pLabels % colorCount][2] * factor;     // r
							*(pTex++) = factor;      						// a
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
					for (int i=0; i < users.getSize(); ++i)
					{
						const nite::UserData& user = users[i];
						updateUserState(user, userTrackerFrame.getTimestamp());

						if (user.isNew())
						{
							std::cout << "NISkeleton got new user ID: " << user.getId() << " startSkeletonTracking "<< std::endl;
							m_pUserTracker->startSkeletonTracking(user.getId());
							//m_pUserTracker->startPoseDetection(user.getId(), nite::POSE_CROSSED_HANDS);
						}
					}
				}

				skelData->update(&userTrackerFrame);
			}

			updt_mtx.unlock();
		}
	}
}

//------------------------------------------------------------------------------

void NISkeleton::drawDebug(ShaderCollector* shCol, camPar* cp)
{
	if(init)
	{
		if(!debugInit)
		{
			/*
			// skeleton colors
			skelColors = new float*[max_users];
			for (int i=0;i<max_users;i++) skelColors[i] = new float[3];

			skelColors[0][0] = 0.f; skelColors[0][1] = 1.f; skelColors[0][2] = 0.f;
			skelColors[1][0] = 0.f; skelColors[1][1] = 1.f; skelColors[1][2] = 0.f;
			skelColors[2][0] = 0.f; skelColors[2][1] = 0.f; skelColors[2][2] = 1.f;
			skelColors[3][0] = 1.f; skelColors[3][1] = 1.f; skelColors[3][2] = 1.f;
			skelColors[4][0] = 0.f; skelColors[4][1] = 1.f; skelColors[4][2] = 0.f;
			skelColors[5][0] = 0.f; skelColors[5][1] = 1.f; skelColors[5][2] = 0.f;
			skelColors[6][0] = 0.f; skelColors[6][1] = 0.f; skelColors[6][2] = 1.f;
			skelColors[7][0] = 1.f; skelColors[7][1] = 1.f; skelColors[7][2] = 1.f;
			skelColors[8][0] = 0.f; skelColors[8][1] = 1.f; skelColors[8][2] = 0.f;
			skelColors[9][0] = 0.f; skelColors[9][1] = 1.f; skelColors[9][2] = 0.f;
*/

			// debug vaos
			pointsVao = new VAO("position:3f,color:4f", GL_DYNAMIC_DRAW);
			pointsVao->initData(50);

			lineLoopVao = new VAO("position:3f,color:4f", GL_DYNAMIC_DRAW);
			lineLoopVao->initData(24);

			skeletonVao = new VAO("position:3f,color:4f", GL_DYNAMIC_DRAW);
			skeletonVao->initData(60);

			g_drawSkeleton = true;
			g_drawCenterOfMass = true;
			g_drawStatusLabel = false;
			g_drawBoundingBox = true;

			g_userStatusLabels = new char*[max_users];
			for (int i=0;i<max_users;i++)
			{
				g_userStatusLabels[i] = new char[100];
				for (int j=0;j<100;j++) g_userStatusLabels[i][j] = 0;
			}

			centerOfMass = new float[3];
			for (unsigned int i=0;i<3;i++)
				centerOfMass[i] = 0;

			boundingBox = new float[8];

			stdCol = shCol->getStdCol();

			debugInit = true;
		}


		const nite::Array<nite::UserData>& users = userTrackerFrame.getUsers();
		for (int i=0; i<users.getSize(); ++i)
		{
			const nite::UserData& user = users[i];

			if (user.isVisible())
			{
				drawPointPtr = 0;
				skelOffPtr = 0;

				glDisable(GL_DEPTH_TEST);
				glm::mat4 pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4;

				//if (g_drawStatusLabel)  DrawStatusLabel(m_pUserTracker, user, &pvm[0][0]);
				if (g_drawCenterOfMass) DrawCenterOfMass(m_pUserTracker, user, &pvm[0][0]);
				if (g_drawBoundingBox)  DrawBoundingBox(user, &pvm[0][0]);
	            if (users[i].getSkeleton().getState() == nite::SKELETON_TRACKED && g_drawSkeleton)
	            	DrawSkeletonFromData(m_pUserTracker, user, &pvm[0][0]);

				glEnable(GL_DEPTH_TEST);
			}
		}
	}
}

//------------------------------------------------------------------------------

void NISkeleton::DrawStatusLabel(nite::UserTracker* pUserTracker, const nite::UserData& user, float* mat)
{
    int color = user.getId() % colorCount;
    float x,y;
    pUserTracker->convertJointCoordinatesToDepth(user.getCenterOfMass().x,
                                                 user.getCenterOfMass().y,
                                                 user.getCenterOfMass().z,
                                                 &x, &y);
    char *msg = g_userStatusLabels[user.getId()];
    //
    //	glRasterPos3f(x / static_cast<float>(resImgWidth) - 0.5f - static_cast<float>(strlen(msg)/2) * 0.01f,
//                      0.5f - ( y / static_cast<float>(resImgHeight) ),
//                      -0.5f);
    //	glPrintString(GLUT_BITMAP_HELVETICA_18, msg);
}

//---------------------------------------------------------------

void NISkeleton::DrawCenterOfMass(nite::UserTracker* pUserTracker, const nite::UserData& user, float* mat)
{
    pUserTracker->convertJointCoordinatesToDepth(user.getCenterOfMass().x,
                                                 user.getCenterOfMass().y,
                                                 user.getCenterOfMass().z,
                                                 &centerOfMass[0],
												 &centerOfMass[1]);

    centerOfMass[0] = float(centerOfMass[0]) / float(resImgWidth) * 2.f - 1.f;
    centerOfMass[1] = float(centerOfMass[1]) / float(resImgHeight) * 2.f - 1.f;
    centerOfMass[2] = 0.f;

    GLfloat* pos = (GLfloat*) pointsVao->getMapBuffer(POSITION);
    for (int i=0;i<3;i++) pos[drawPointPtr*3 +i] = centerOfMass[i];
    pointsVao->unMapBuffer();

    GLfloat* color = (GLfloat*) pointsVao->getMapBuffer(COLOR);
    color[drawPointPtr*4] = 1.f;
    color[drawPointPtr*4 +1] = 1.f;
    color[drawPointPtr*4 +2] = 0.f;
    color[drawPointPtr*4 +3] = 1.f;
    pointsVao->unMapBuffer();

    glPointSize(8);

    stdCol->begin();
    stdCol->setUniformMatrix4fv("m_pvm", mat);
    pointsVao->draw(GL_POINTS, drawPointPtr, 1, nullptr, GL_POINTS);
    stdCol->end();

    drawPointPtr++;
}

//---------------------------------------------------------------

void NISkeleton::DrawBoundingBox(const nite::UserData& user, float* mat)
{
    boundingBox[0] = (user.getBoundingBox().max.x / static_cast<float>(resImgWidth)) * 2.f - 1.f;
    boundingBox[1] = 1.f - (user.getBoundingBox().max.y / static_cast<float>(resImgHeight)) * 2.f;

    boundingBox[2] = (user.getBoundingBox().max.x / static_cast<float>(resImgWidth)) * 2.f - 1.f;
    boundingBox[3] = 1.f - (user.getBoundingBox().min.y / static_cast<float>(resImgHeight)) * 2.f;

    boundingBox[4] = (user.getBoundingBox().min.x / static_cast<float>(resImgWidth)) * 2.f - 1.f;
    boundingBox[5] = 1.f - (user.getBoundingBox().min.y / static_cast<float>(resImgHeight)) * 2.f;

    boundingBox[6] = (user.getBoundingBox().min.x / static_cast<float>(resImgWidth)) * 2.f - 1.f;
    boundingBox[7] = 1.f - (user.getBoundingBox().max.y / static_cast<float>(resImgHeight)) * 2.f;


    GLfloat* pos = (GLfloat*) lineLoopVao->getMapBuffer(POSITION);
    for (int i=0;i<4;i++)
    {
        pos[i*3] = boundingBox[i*2];
        pos[i*3+1] = boundingBox[i*2 +1];
        pos[i*3+2] = 0.f;
    }
    lineLoopVao->unMapBuffer();


    GLfloat* color = (GLfloat*) lineLoopVao->getMapBuffer(COLOR);
    for (int i=0;i<4;i++)
    {
        color[i*4] = 1.f; color[i*4+1] = 1.f; color[i*4+2] = 1.f; color[i*4+3] = 1.f;
    }
    lineLoopVao->unMapBuffer();


    glPointSize(2);
    stdCol->begin();
    stdCol->setUniformMatrix4fv("m_pvm", mat);
    lineLoopVao->draw(GL_LINE_LOOP, 0, 4, nullptr, GL_LINE_LOOP);
}

//---------------------------------------------------------------

void NISkeleton::DrawLimbFromData(short userId, nite::JointType joint1, nite::JointType joint2,
	int color, float* mat)
{
    float coordinates[4] = {-0.5f, -0.5f, -0.5f, -0.5f};

    glm::vec3 p1 = skelData->getJoint(userId, joint1);
    glm::vec3 p2 = skelData->getJoint(userId, joint2);

    // write lines into the vao
    GLfloat* pos = (GLfloat*) skeletonVao->getMapBuffer(POSITION);
    pos[skelOffPtr*3] = p1.x; pos[skelOffPtr*3+1] = p1.y; pos[skelOffPtr*3+2] = 0.f;
    pos[skelOffPtr*3+3] = p2.x; pos[skelOffPtr*3+4] = p2.y; pos[skelOffPtr*3+5] = 0.f;
    skeletonVao->unMapBuffer();

    GLfloat* colorPtr = (GLfloat*) skeletonVao->getMapBuffer(COLOR);
    for (int i=0;i<2;i++){
        colorPtr[(i+skelOffPtr)*4] = Colors[color][0];
        colorPtr[(i+skelOffPtr)*4+1] = Colors[color][1];
        colorPtr[(i+skelOffPtr)*4+2] = Colors[color][2];
        colorPtr[(i+skelOffPtr)*4+3] = 1.f;
    }
    skeletonVao->unMapBuffer();

    // draw the lines
    skeletonVao->draw(GL_LINES, skelOffPtr, 2, nullptr, GL_LINES);
    skelOffPtr += 2;

    // write points into vao
    GLfloat* pPos = (GLfloat*) pointsVao->getMapBuffer(POSITION);
    pPos[drawPointPtr*3] = p1.x; pPos[drawPointPtr*3+1] = p1.y; pPos[drawPointPtr*3+2] = 0.f;
    pPos[drawPointPtr*3+3] = p2.x; pPos[drawPointPtr*3+4] = p2.y; pPos[drawPointPtr*3+5] = 0.f;
    pointsVao->unMapBuffer();

    GLfloat* pColorPtr = (GLfloat*) pointsVao->getMapBuffer(COLOR);
    for (int i=0;i<2;i++){
        pColorPtr[(i+drawPointPtr)*4] = Colors[color][0];
        pColorPtr[(i+drawPointPtr)*4+1] = Colors[color][1];
        pColorPtr[(i+drawPointPtr)*4+2] = Colors[color][2];
        pColorPtr[(i+drawPointPtr)*4+3] = 1.f;
    }
    pointsVao->unMapBuffer();

    glPointSize(10);
    stdCol->begin();
    stdCol->setUniformMatrix4fv("m_pvm", mat);
    pointsVao->draw(GL_POINTS, drawPointPtr, 2, nullptr, GL_POINTS);

    drawPointPtr += 2;
}

//---------------------------------------------------------------

void NISkeleton::DrawSkeletonFromData(nite::UserTracker* pUserTracker, const nite::UserData& userData,
	float* mat)
{
    for (auto i=0;i<skelData->getNrUsers();i++)
    {
        DrawLimbFromData(i, nite::JOINT_HEAD, nite::JOINT_NECK, i % colorCount, mat);
        DrawLimbFromData(i, nite::JOINT_LEFT_SHOULDER, nite::JOINT_LEFT_ELBOW, i % colorCount, mat);
        DrawLimbFromData(i, nite::JOINT_LEFT_ELBOW, nite::JOINT_LEFT_HAND, i % colorCount, mat);
        DrawLimbFromData(i, nite::JOINT_RIGHT_SHOULDER, nite::JOINT_RIGHT_ELBOW, i % colorCount, mat);
        DrawLimbFromData(i, nite::JOINT_RIGHT_ELBOW, nite::JOINT_RIGHT_HAND, i % colorCount, mat);
        DrawLimbFromData(i, nite::JOINT_LEFT_SHOULDER, nite::JOINT_RIGHT_SHOULDER, i % colorCount, mat);
        DrawLimbFromData(i, nite::JOINT_LEFT_SHOULDER, nite::JOINT_TORSO, i % colorCount, mat);
        DrawLimbFromData(i, nite::JOINT_RIGHT_SHOULDER, nite::JOINT_TORSO, i % colorCount, mat);
        DrawLimbFromData(i, nite::JOINT_TORSO, nite::JOINT_LEFT_HIP, i % colorCount, mat);
        DrawLimbFromData(i, nite::JOINT_TORSO, nite::JOINT_RIGHT_HIP, i % colorCount, mat);
        DrawLimbFromData(i, nite::JOINT_LEFT_HIP, nite::JOINT_RIGHT_HIP, i % colorCount, mat);
        DrawLimbFromData(i, nite::JOINT_LEFT_HIP, nite::JOINT_LEFT_KNEE, i % colorCount, mat);
        DrawLimbFromData(i, nite::JOINT_LEFT_KNEE, nite::JOINT_LEFT_FOOT, i % colorCount, mat);
        DrawLimbFromData(i, nite::JOINT_RIGHT_HIP, nite::JOINT_RIGHT_KNEE, i % colorCount, mat);
        DrawLimbFromData(i, nite::JOINT_RIGHT_KNEE, nite::JOINT_RIGHT_FOOT, i % colorCount, mat);
    }
}

//------------------------------------------------------------------------------

void NISkeleton::updateUserState(const nite::UserData& user, uint64_t ts)
{
	if (user.isNew())
	{
		std::cout << "New User" << std::endl;
		actUser = (short) (user.getId() - 1);
	}
	else if (user.isVisible() && !g_visibleUsers[user.getId()]) {

		printf("[%08" PRIu64 "] User #%d:\tVisible\n", ts, user.getId());

	} else if (!user.isVisible() && g_visibleUsers[user.getId()]) {

		printf("[%08" PRIu64 "] User #%d:\tOut of SceneNode\n", ts,
				user.getId());

	} else if (user.isLost()) {

		std::cout << ("Lost");
	}
	g_visibleUsers[user.getId()] = user.isVisible();

}

//------------------------------------------------------------------------------

void NISkeleton::onKey(int key, int scancode, int action, int mods)
{
}

//------------------------------------------------------------------------------

void NISkeleton::Finalize()
{
	userTrackerReady = false;
	nite::NiTE::shutdown();
	//delete m_pUserTracker;
}

//------------------------------------------------------------------------------

void NISkeleton::reset()
{
	updt_mtx.lock();

	userTrackerReady = false;

	if (!niteDestroyed)
	{
		userTrackerFrame.release();
		m_pUserTracker->destroy();
		nite::NiTE::shutdown();
		delete m_pUserTracker;
		niteDestroyed = true;
	}

	// start new init
	nite::NiTE::initialize();
	m_pUserTracker = new nite::UserTracker;
	if (m_pUserTracker->create(dev) != nite::STATUS_OK)
		std::cout << "NISkeleton: failed to open Kinect device" << std::endl;

	// refresh references
	skelData->setUserTracker(m_pUserTracker);

	userTrackerReady = true;
	niteDestroyed = false;

	updt_mtx.unlock();
}

//------------------------------------------------------------------------------

uint8_t* NISkeleton::getResImg()
{
	uint8_t* out = 0;

	updt_mtx.lock();
	out = res_img;
	updt_mtx.unlock();

	return out;
}

//------------------------------------------------------------------------------

float NISkeleton::getAvgAct(short userId)
{
	return skelData->getAvgAct(userId);
}

//------------------------------------------------------------------------------

glm::vec3 NISkeleton::getJoint(short userId, nite::JointType type)
{
	return skelData->getJoint(userId, type);
}

//------------------------------------------------------------------------------

glm::vec3 NISkeleton::getJointDir(short userId, nite::JointType type)
{
	return skelData->getJointDir(userId, type);
}

//------------------------------------------------------------------------------

short NISkeleton::getMostActiveUser()
{
	return skelData->getMostActiveUser();
}

//------------------------------------------------------------------------------

short NISkeleton::getNearestUser()
{
	return skelData->getNearestUser();
}

//------------------------------------------------------------------------------

const nite::UserMap& NISkeleton::getUserMap()
{
	return userTrackerFrame.getUserMap();
}

//------------------------------------------------------------------------------

nite::UserTrackerFrameRef* NISkeleton::getUserTrackerFrame()
{
	nite::UserTrackerFrameRef* out = 0;

	if (userTrackerFrame.isValid())
		out = &userTrackerFrame;

	return out;
}

//------------------------------------------------------------------------------

nite::UserTracker* NISkeleton::getUserTracker()
{
	return m_pUserTracker;
}

//------------------------------------------------------------------------------

void NISkeleton::setUpdateImg(bool _set)
{
	updateImg = _set;
}

//------------------------------------------------------------------------------

void NISkeleton::setUpdateSkel(bool _set)
{
	updateSkeleton = _set;
}

//------------------------------------------------------------------------------

int NISkeleton::getFrameNr()
{
	return frameNr;
}

//------------------------------------------------------------------------------

NISkeletonData* NISkeleton::getSkelData()
{
	return skelData;
}

//------------------------------------------------------------------------------

NISkeleton::~NISkeleton()
{
	Finalize();
	delete res_img;

	if(debugInit)
	{
	 //   delete [] skelColors;
	    delete pointsVao;
	    delete lineLoopVao;
		delete skeletonVao;
        delete centerOfMass;
        delete boundingBox;
	}
}

//------------------------------------------------------------------------------

bool NISkeleton::isInited()
{
	return init;
}

//------------------------------------------------------------------------------

bool NISkeleton::isReady()
{
	return ready;
}

//------------------------------------------------------------------------------

void NISkeleton::myNanoSleep(uint32_t ns)
{
	struct timespec tim;
	tim.tv_sec = 0;
	tim.tv_nsec = (long) ns;
	nanosleep(&tim, NULL);
}

}
