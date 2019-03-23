/*
 *  VideoTextureCv.cpp
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 29.05.11.
 *  Copyright 2011 Sven Hahne. All rights reserved.
 *
 *  problem: wenn threads vom opengl thread gestartet werden passiert murks,
 *  deshalb thread am anfang starten und per flag aktivieren oder nur warten lassen
 *
 * !!!!!!!!!!! Im setup muss bei diesem Scenenode ein vtex0 angegeben werden !!!!!!!!!!!!!!!!
 *
 * wenn cuda aktiviert ist, wird das cudadecoder modul von opencv verwendet, sollte die
 * video hardware decodierung von nvidia grafikkarten benutzen, kommt bei 4k videos noch komplett durcheinander
 *
 *   */

#include "VideoTextureCv.h"



namespace tav {

VideoTextureCv::VideoTextureCv(const char * file)
{
#ifdef HAVE_OPENCV

	ready = false;
	nrBufferFrames = 3;
	preBufNrCvFrames = 1;
	actBuffer=0;
	prebufferedCvFrames=0;
	cvReadFramePtr = 0;
	pboWriteFramePtr = 0;
	lastUploadedFrame = 0;
	actCvFrame = 0;


	cap.open(file); // open the default camera
	if(!cap.isOpened())  // check if we succeeded
	   std::cout << "VideoTextureCv couldn't open video" << std::endl;

	width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
	height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
	fps = cap.get(cv::CAP_PROP_FPS);
	cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('H', '2', '6', '4'));

	std::cout << "video " << file << "  opened size: " << width << ": " << height << std::endl;
	frameTime = 1.0 / double(fps);

	vtFrames = new vtFrame[nrBufferFrames];
	for (int i=0;i<nrBufferFrames;i++)
	{
		vtFrames[i].uploaded = false;
		vtFrames[i].matFilled = false;

		vtFrames[i].tex.allocate(width, height, GL_RGBA8, GL_RGBA, GL_TEXTURE_2D);
		vtFrames[i].tex.setWraping(GL_CLAMP_TO_EDGE);
		vtFrames[i].tex.setFiltering(GL_LINEAR, GL_LINEAR);
		vtFrames[i].mat = cv::Mat(height, width, CV_8UC3);
	}

	zeroTime = glfwGetTime();
	run = true;
	ready = true;
	thread = new std::thread(&VideoTextureCv::processQueue, this);
	thread->detach();
#endif
}

//------------------------------------------------------------------------------------

VideoTextureCv::~VideoTextureCv()
{
#ifdef HAVE_OPENCV
	// Free the RGB image
	delete [] vtFrames;

#endif
}

//------------------------------------------------------------------------------------

#ifdef HAVE_OPENCV

void VideoTextureCv::open(const char * file)
{
	mutex.lock();

	run = false;
	ready = false;

	if (thread && thread->joinable())
	thread->join();

	if(cap.isOpened()) cap.release();
	//delete cap;
	cap.open(file);
	cap.set(cv::CAP_PROP_POS_FRAMES, 0);

	run = true;
	ready = true;
	cvReadFramePtr = 0;
	lastUploadedFrame = 0;
	prebufferedCvFrames = 0;

	zeroTime = glfwGetTime();

	delete thread;
	thread = new std::thread(&VideoTextureCv::processQueue, this);
	thread->detach();

	mutex.unlock();

}

//------------------------------------------------------------------------------------

void VideoTextureCv::resetToStart()
{
	mutex.lock();

	zeroTime = glfwGetTime();
	bool success = cap.set(cv::CAP_PROP_POS_FRAMES, 0);
	//prebufferedCvFrames = 0;

	if (!run){
		prebufferedCvFrames = 0;
		lastUploadedFrame = 0;
		cvReadFramePtr = 0;

		run = true;
		thread = new std::thread(&VideoTextureCv::processQueue, this);
		thread->detach();
	}
	mutex.unlock();
}

//------------------------------------------------------------------------------------

void VideoTextureCv::updateDt(double time, bool startStop)
{

	/*
	//update background video
    if ( !processing && cap.get(cv::CAP_PROP_POS_MSEC) * 0.001 < (time - zeroTime))
    {
    	if (cap->get(cv::CAP_PROP_POS_FRAMES) > cap->get(cv::CAP_PROP_FRAME_COUNT) -3)
    	{
    		cap->set(cv::CAP_PROP_POS_FRAMES, 0);
    		zeroTime = time;
    	}

    	decodeCond.notify_one();
    }
    */

}

//------------------------------------------------------------------------------------

void VideoTextureCv::set_paused(bool new_value)
{
}

//------------------------------------------------------------------------------------

void VideoTextureCv::stop()
{
	run = false;
	cap.release();
	//if (thread->joinable())
	//	thread->join();
}

//------------------------------------------------------------------------------------

bool VideoTextureCv::isPausing()
{
	return false;
}

//------------------------------------------------------------------------------------

void VideoTextureCv::processQueue()
{
	std::unique_lock<std::mutex> lock(pp_mutex);

	while(run)
	{
		actCvFrame = cap.get(cv::CAP_PROP_POS_MSEC);
		unsigned int actTimeGlfw = (glfwGetTime() - zeroTime) * 1000.0;

		//std::cout << "actCvFrame: " << actCvFrame << std::endl;
		//std::cout << "actTimeGlfw: " << actTimeGlfw << std::endl;

		//std::cout << "glfwGetTime(): " << glfwGetTime() << " zeroTime: " << zeroTime << " (glfwGetTime() - zeroTime):" << (glfwGetTime() - zeroTime);
		//std::cout << " frameTime:" << frameTime << std::endl;
		//std::cout << "actCvFrame: " << actCvFrame << " actTimeGlfw: " << actTimeGlfw << std::endl;

		// if we have less prebuffered frames than needed, decode
		if (prebufferedCvFrames < nrBufferFrames)
		{
			// loop
			bool isEnd = cap.get(cv::CAP_PROP_POS_FRAMES) > cap.get(cv::CAP_PROP_FRAME_COUNT) -3;

			if (isEnd)
			{
				if (loop)
				{
					cap.set(cv::CAP_PROP_POS_FRAMES, 0);
		    		zeroTime = glfwGetTime();
		    		actCvFrame = 0;
				} else {
					run = false;
					break;
				}
	    	}

			mutex.lock();

			cap >> vtFrames[cvReadFramePtr].mat; 							// decode frame
			vtFrames[cvReadFramePtr].pts = actCvFrame; 	// save timestamp
			vtFrames[cvReadFramePtr].uploaded = false;
			vtFrames[cvReadFramePtr].matFilled = true;
			cvReadFramePtr = (cvReadFramePtr +1) % nrBufferFrames;

			mutex.unlock();

			//std::cout << "prebufferedCvFrames " << cvReadFramePtr << std::endl;
			prebufferedCvFrames++;

		} else {

			// if we have enough lock the thread and wait for unlocking
			decodeCond.wait(lock);
		}
	}

	run = false; // set it, in case we are breaking the while loop, because of file end

	std::cout << "process queue ended" << std::endl;
}

//------------------------------------------------------------------------------------

void VideoTextureCv::loadFrameToTexture(double time)
{
   // std::cout << "prebufferedCvFrames " << prebufferedCvFrames << std::endl;

	unsigned int actTimeGlfw = (glfwGetTime() - zeroTime) * 1000.0;

	if ( ready && run &&  prebufferedCvFrames > 0 )
	{
		// from the actual cvReadFramePtr (if all filled points to the oldest decoded frame)
		// look for the next not uploaded frame and upload
		mutex.lock();

		unsigned int fptr=0;
		bool found = false;
		unsigned int checkPtr;

		while(fptr < nrBufferFrames && !found)
		{
			checkPtr = (cvReadFramePtr + fptr) % nrBufferFrames;
			if (!vtFrames[checkPtr].uploaded && vtFrames[checkPtr].matFilled
					&& actTimeGlfw - vtFrames[checkPtr].pts > 10)
			{
				found = true;
				break;
			}
			fptr++;
		}

		if (!found)
		{
			mutex.unlock();

			//std::cout << "not found" << std::endl;
			return;
		}

		//std::cout << "got frame with time : " << vtFrames[checkPtr].pts;
		//std::cout << " act time: " << actTimeGlfw << std::endl;

	    glBindTexture(GL_TEXTURE_2D,  vtFrames[checkPtr].tex.getId());
	    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGR,
	    		GL_UNSIGNED_BYTE, &vtFrames[checkPtr].mat.data[0]);

	    vtFrames[checkPtr].uploaded = true;
	    vtFrames[checkPtr].matFilled = false;

	    prebufferedCvFrames--;

    	decodeCond.notify_one();

	    lastUploadedFrame = checkPtr;

	    mutex.unlock();

	} else {
		mutex.unlock();
	}
}

//------------------------------------------------------------------------------------

GLint VideoTextureCv::getTex()
{
	return vtFrames[lastUploadedFrame].tex.getId();
}

//------------------------------------------------------------------------------------

void VideoTextureCv::bindActFrame(unsigned int texUnit)
{
	glActiveTexture(GL_TEXTURE0 +texUnit);
	vtFrames[actTexBuf].tex.bind(texUnit);
}

//------------------------------------------------------------------------------------

cv::Mat* VideoTextureCv::getActMat()
{
	cv::Mat* out;

	if (!processing)
		out = &vtFrames[actFramePtr].mat;
	else
		out = &vtFrames[(actFramePtr -1 + nrBufferFrames) % nrBufferFrames].mat;

	return out;
}

//------------------------------------------------------------------------------------

unsigned int VideoTextureCv::getWidth(){
	return width;
}

//------------------------------------------------------------------------------------

unsigned int VideoTextureCv::getHeight()
{
	return height;
}

//------------------------------------------------------------------------------------

int VideoTextureCv::getActFrame()
{
	return frame_count;
}

#endif
}
