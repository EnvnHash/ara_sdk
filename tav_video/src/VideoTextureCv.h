/*
 *  VideoTextureCv.h
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 29.05.11.
 *  Copyright 2011 Sven Hahne. All rights reserved.
 *
 */

#pragma once

#ifndef _VideoTextureCv_
#define _VideoTextureCv_

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>


#include "GLUtils/TextureManager.h"

#ifdef HAVE_OPENCV
#include <headers/opencv_headers.h>
#endif
#include <GLFW/glfw3.h>


namespace tav
{
typedef struct {
	GLuint			pbo;
	TextureManager	tex;
	double 			pts;
#ifdef HAVE_OPENCV
	cv::Mat			mat;
#endif
	bool			uploaded;
	bool			matFilled;
} vtFrame;

class VideoTextureCv
{
public :
	VideoTextureCv(const char * file);
	~VideoTextureCv();

#ifdef HAVE_OPENCV
	void open(const char * file);

	void set_loop(bool _val) {}


	void stop();
	void resetToStart();
	void updateDt(double newDt, bool startStop=false);
	void set_paused(bool new_value);
	virtual void processQueue();
	void get_frame_pointer(double time, bool loop=true);
	void loadFrameToTexture(double time);
	bool isPausing();
	GLint getTex();
	void bindActFrame(unsigned int texUnit=0);
	cv::Mat* getActMat();
	unsigned int getWidth();
	unsigned int getHeight();
	int getActFrame();
	bool isReady() { return ready; }
	void setLoop(bool _val) { loop = _val; }

	int nrBufferFrames;
	int actBuffer;
	int textureSize;
#endif

private:

	vtFrame* 			vtFrames;
	std::mutex        	pp_mutex;
	std::mutex        	mutex;
	std::thread*      	thread;
	std::thread 		m_Thread;

#ifdef HAVE_OPENCV
	cv::VideoCapture	cap;
#endif

	std::condition_variable decodeCond;
	bool                run=true;
	bool				processing=false;
	bool				ready = false;
	bool				loop = false;

	int                 frame_count = 0;
	int					lastUploadedFrame = 0;
	int					actTexBuf = -1;
	int					actFramePtr = -1;

	unsigned int		width;
	unsigned int		height;
	unsigned int		fps;

	unsigned int 		actCvFrame;
	unsigned int		prebufferedCvFrames;
	unsigned int		cvReadFramePtr;
	unsigned int		pboWriteFramePtr;
	unsigned int		preBufNrCvFrames;

	double				zeroTime=0;
	double				frameTime=0;
};
}

#endif
