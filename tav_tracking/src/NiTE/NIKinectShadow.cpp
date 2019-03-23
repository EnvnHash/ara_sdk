//
//  NIKinectShadow.cpp
//  Tav_App
//
//  Created by Sven Hahne on 10/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//
//  mehrere Threads ist eigentlich quatsch, weil die threads eventuell
//  auf diesselbe stellen schreiben muessen und sich gegenseitig ausbremsen...
//  macht nur sinn um den DepthStreamListener und alle andreren Postprocs nicht
//  zu blockieren

#include "NiTE/NIKinectShadow.h"

namespace tav
{

NIKinectShadow::NIKinectShadow(int _width, int _height, int _nrChans,
		kinectMapping* _kMap, nite::UserTrackerFrameRef* _userTrackerFrame) :
				KinectInputPostProc(), width(_width), height(_height), nrChans(
						_nrChans), kMap(_kMap), userTrackerFrame(_userTrackerFrame), useThreads(
								true), nrThreads(1), maxLineMult(4), shadow_pp_ptr(0), nrShadowBufs(
										3), shadow_tBlend(0.6f), fpsInt(10), fpsCnt(0), cropLeft(
												-5000.f), cropRight(5000.f), cropDepth(8000.f)
{
	shadow_pp = new uint8_t*[nrShadowBufs];
	shadow_pp_frame_map = new int[nrShadowBufs];
	for (int i = 0; i < nrShadowBufs; i++)
	{
		shadow_pp_frame_map[i] = 0;
		shadow_pp[i] = new uint8_t[width * height * nrChans];
		memset(shadow_pp[i], 0, width * height * nrChans * sizeof(uint8_t));
	}

	threads = new boost::thread*[nrThreads];
	ready = true;
}

//------------------------------------------------------------------------------

void NIKinectShadow::update(openni::VideoStream& _stream,
		openni::VideoFrameRef* _frame)
{
	float maxXDist = kMap->distScale->x * kMap->scale->x;
	float maxZDist = kMap->distScale->y * kMap->scale->y;

	if (userTrackerFrame->isValid() && _frame->isValid() && _stream.isValid()
			&& doUpdate)
	{
		*_frame = userTrackerFrame->getDepthFrame();

		// get user centers
		const nite::Array<nite::UserData>& users = userTrackerFrame->getUsers();

		if ((unsigned short) userCenters.size()
				< (unsigned short) users.getSize())
			userCenters.push_back(glm::vec3(0.f));

		if ((unsigned short) userCenters.size()
				>= (unsigned short) users.getSize())
			userCenters.resize(users.getSize());

		for (short i = 0; i < users.getSize(); i++)
		{
			userCenters[i].x = users[i].getCenterOfMass().x;
			userCenters[i].y = users[i].getCenterOfMass().y;
			userCenters[i].z = users[i].getCenterOfMass().z;

			userCenters[i].y = 1.f
					- std::fmin(std::fmax(userCenters[i].z / maxZDist, 0.0),
							0.95);
			userCenters[i].y += kMap->offset->y;

			userCenters[i].x = std::fmin(
					std::fmax((userCenters[i].x + maxXDist * 0.5f) / maxXDist,
							0.0), 0.95);
			userCenters[i].x += kMap->offset->x;
		}

		const nite::UserMap& userLabels = userTrackerFrame->getUserMap();

		if (useThreads && !threadRunning)
		{
			shadow_pp_ptr = (shadow_pp_ptr + 1) % nrShadowBufs;

			// start threads
			for (short i = 0; i < nrThreads; i++)
				threads[i] = new boost::thread(&NIKinectShadow::processQueue,
						this, i, &_stream, _frame, userLabels);

			// wait until there all finished
			for (short i = 0; i < nrThreads; i++)
			{
				threads[i]->join();
				delete threads[i];
			}

			frameNr++;
			shadow_pp_frame_map[shadow_pp_ptr] = frameNr;

		}
		else
		{
			float factor = 1;
			float wX, wY, wZ;
			int iwX, iwZ, addNrLines;
			int rowNr;
			uint8_t* pShadowNew;
			uint8_t* pShadowOld;

			float maxXDist = kMap->distScale->x * kMap->scale->x;
			float maxZDist = kMap->distScale->y * kMap->scale->y;

			const nite::UserId* pLabels = userLabels.getPixels();
			const openni::DepthPixel* pDepthRow =
					(const openni::DepthPixel*) _frame->getData();

			int rowSize = _frame->getStrideInBytes()
							/ sizeof(openni::DepthPixel);
			int colRowSize = width * nrChans;

			shadow_pp_ptr = (shadow_pp_ptr + 1) % nrShadowBufs;
			frameNr++;
			shadow_pp_frame_map[shadow_pp_ptr] = frameNr;

			memset(shadow_pp[shadow_pp_ptr], 0,
					width * height * nrChans * sizeof(uint8_t));

			for (int y = 0; y < _frame->getHeight(); ++y)
			{
				const openni::DepthPixel* pDepth = pDepthRow; // read first depth pixel at actual row

				for (int x = 0; x < _frame->getWidth();
						++x, ++pDepth, ++pLabels)
				{
					// get realworld coordinates
					openni::CoordinateConverter::convertDepthToWorld(_stream, x,
							y, *pDepth, &wX, &wY, &wZ);
					wZ = 1.f - std::fmin(std::fmax(wZ / maxZDist, 0.0), 0.95);
					wX = std::fmin(
							std::fmax((wX + maxXDist * 0.5f) / maxXDist, 0.0),
							0.95);

					addNrLines = static_cast<int>(std::fmax(
							(1.f - wZ) * (float) maxLineMult, 1.f));

					// since the depth data seems to be rasterized
					// add some random to the z-coordinate
					// smooth the data by adding the last frame weighted
					iwX = static_cast<int>((wX + kMap->offset->x)
							* (float) width + 0.5f);
					rowNr = static_cast<int>((wZ + kMap->offset->y)
							* (float) height);
					iwZ = std::min(rowNr * width, (height - 1) * width);

					addNrLines = std::max(
							std::min(rowNr + addNrLines, height - 1) - rowNr,
							1);

					pShadowNew = shadow_pp[shadow_pp_ptr]
										   + (iwX + iwZ) * nrChans;

					for (int c = 0; c < 4; c++)
					{
						for (int l = 0; l < addNrLines; l++)
							*(pShadowNew + l * colRowSize) = factor;
						pShadowNew++;
					}
				}

				pDepthRow += rowSize;   // count up one row
			}
		}
	}
}

//------------------------------------------------------------------------------

void NIKinectShadow::processQueue(unsigned int N,
		openni::VideoStream* depthStream, openni::VideoFrameRef* depthFrame,
		const nite::UserMap& userLabels)
{
	float wX, wY, wZ;
	int iwX, iwZ, addNrLines, rowNr;
	uint8_t* pShadowNew;
	uint8_t* pShadowOld;

	float maxXDist = kMap->distScale->x * kMap->scale->x;
	float maxZDist = kMap->distScale->y * kMap->scale->y;

	int nrRows = height / nrThreads;
	int rowOffset = N * nrRows;
	int rowSize = depthFrame->getStrideInBytes() / sizeof(openni::DepthPixel);
	int colRowSize = width * nrChans;

	const nite::UserId* pLabels = userLabels.getPixels();
	pLabels += rowOffset * width;

	const openni::DepthPixel* pDepthRow =
			(const openni::DepthPixel*) depthFrame->getData();
	pDepthRow += rowOffset * rowSize;

	// reset shadow_tex
	memset(shadow_pp[shadow_pp_ptr] + rowOffset * width * nrChans, 0,
			width * height * nrChans * sizeof(uint8_t) / nrThreads);

	float factor = 255;

	for (int y = 0; y < nrRows; ++y)
	{
		const openni::DepthPixel* pDepth = pDepthRow; // read first depth pixel at actual row

		for (int x = 0; x < depthFrame->getWidth(); ++x, ++pDepth, ++pLabels)
		{
			if (*pLabels != 0 && *pDepth != 0)
			{
				// get realworld coordinates
				openni::CoordinateConverter::convertDepthToWorld(*depthStream,
						x, y + rowOffset, *pDepth, &wX, &wY, &wZ);

				if (wX > cropLeft && wX < cropRight)
				{
					wZ = 1.f - std::fmin(std::fmax(wZ / maxZDist, 0.0), 1.f);
					wX = std::fmin(
							std::fmax(
									(wX + maxXDist * 0.5f) / maxXDist
									+ getRandF(-0.001, 0.001)
									* (1.f - wZ), 0.0), 0.95);
					addNrLines = static_cast<int>(std::fmax(
							(1.f - wZ) * (float) maxLineMult, 1.f));

					// since the depth data seems to be rasterized
					// add some random to the z-coordinate
					// smooth the data by adding the last frame weighted
					iwX = static_cast<int>((wX + kMap->offset->x)
							* (float) width + 0.5f);
					rowNr = std::min(
							static_cast<int>((wZ + kMap->offset->y)
									* (float) height), height - 1);
					iwZ = rowNr * width;
					addNrLines = std::max(
							std::min(rowNr + addNrLines, height - 1) - rowNr,
							1);

					pShadowNew = shadow_pp[shadow_pp_ptr]
										   + (iwX + iwZ) * nrChans;

					factor = (wY + 1000.f) * 0.1275f;
					factor = std::fmax(factor, *pShadowNew);

					for (int c = 0; c < nrChans; c++)
					{
						for (int l = 0; l < addNrLines; l++)
							*(pShadowNew + l * colRowSize) = (
									c == 3 ? 255 : factor);
						pShadowNew++;
					}
				}
			}
		}

		pDepthRow += rowSize;   // count up one row
	}
}

//------------------------------------------------------------------------------

bool NIKinectShadow::isReady()
{
	return ready;
}

//------------------------------------------------------------------------------

uint8_t* NIKinectShadow::getShadowImg(int frameNr)
{
	uint8_t* out = 0;
	for (int i = 0; i < nrShadowBufs; i++)
		if (shadow_pp_frame_map[i] == frameNr)
			out = &shadow_pp[i][0];

	return out;
}

//------------------------------------------------------------------------------

int NIKinectShadow::getFrameNr()
{
	return frameNr;
}

//------------------------------------------------------------------------------

int NIKinectShadow::getBufPtr()
{
	return (shadow_pp_ptr - 1 + nrShadowBufs) % nrShadowBufs;
}

//------------------------------------------------------------------------------

float NIKinectShadow::getRandF(float min, float max)
{
	float outVal = 0.0f;
	outVal = rand() % 100000;
	outVal *= 0.00001f;

	outVal *= max - min;
	outVal += min;

	return outVal;
}

//------------------------------------------------------------------------------

glm::vec3* NIKinectShadow::getUserCenter(short userNr)
{
	glm::vec3* out = 0;
	if (short(userCenters.size()) - 1 >= userNr)
		out = &userCenters[userNr];
	return out;
}

//------------------------------------------------------------------------------

NIKinectShadow::~NIKinectShadow()
{
	delete[] shadow_pp;
}

}
