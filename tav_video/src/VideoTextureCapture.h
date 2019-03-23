/*
 *  VideoTextureCapture.h
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 29.05.11.
 *  Copyright 2011 Sven Hahne. All rights reserved.
 *
 */


#ifndef _VideoTextureCapture_
#define _VideoTextureCapture_

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory>
#include <vector>

#include "GLUtils/TextureManager.h"

#include <thread>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}


namespace tav {
class VideoTextureCapture
{
public :
	VideoTextureCapture(char* device, int initFrameRate);
	~VideoTextureCapture();
	int stream_total_frames;
	int stream_width;
	int stream_height;
	int cur_frame;
	int last_frame;
	int stream_index;
	int stream_nr;
	int frame_num_bytes;
	int nrBufferFrames;
	int actBuffer;
	int stream_msec_per_frame;
	void get_frame_pointer(double time, bool loop=true);
	//            void set_frame(double time, bool loop);
	void loadFrameToTexture();
	GLuint* texIDs;
	int textureSize;

	void stopReset();
	void updateDt(double newDt, bool startStop);
	void join();
	virtual void processQueue();

private:
	std::mutex        mutex;
	std::thread*      m_Thread;
	SwsContext*         img_convert_ctx;

	AVFormatContext*    pFormatCtx;
	AVInputFormat*		pFormat;
	AVPixelFormat       dst_pix_fmt;

	unsigned int        i, videoStream;
	AVStream*           videoStream2;


	AVCodecContext*     pCodecCtx;
	AVCodec*            pCodec;
	/*
            AVStream*		stream;
            AVMediaType		codecType;
            int				codecID;
            AVCodecContext	*avVideoCodec;
            std::vector<uint8_t> codecContextExtraData;
	 */
	AVFrame*            pFrame;
	AVFrame**           pFrameRGB;
	AVPacket            packet;
	int                 frameFinished;
	int                 numBytes;
	uint8_t**           buffer;
	TextureManager*     textures;
	int                 frameRate = 0;

	bool                isRunning = false;
	bool                requestStop = false;

	double              internTime = 0.0;
	double              actDt = 0.0;

	int                 lastReadFrame = -1;
	int                 nrFramesBuffered = 0;
	int                 readBuffer = 1;
	int                 waitReadBuf = 0;
};
}

#endif
