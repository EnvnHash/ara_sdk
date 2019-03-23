/*
 *  VideoTextureThread.h
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 29.05.11.
 *  Copyright 2011 Sven Hahne. All rights reserved.
 *
 */

#pragma once

#ifndef _VideoTextureThread_
#define _VideoTextureThread_

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory>
#include <vector>
#include <condition_variable>

#include "GLUtils/TextureManager.h"

#include <thread>
#include <mutex>

extern "C" {
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/avutil.h>
	#include <libavutil/channel_layout.h>
	#include <libavutil/common.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/mathematics.h>
	#include <libavutil/opt.h>
	#include <libavutil/samplefmt.h>
}

#define INBUF_SIZE 4096


namespace tav
{
    class VideoTextureThread
    {
        public :
            VideoTextureThread(char * file, int initFrameRate);
            ~VideoTextureThread();

            void stopReset();
            void updateDt(double newDt, bool startStop);
            void set_paused(bool new_value);
            virtual void processQueue();
            void get_frame_pointer(double time, bool loop=true);
            void loadFrameToTexture();
            bool isPausing();

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
//            void set_frame(double time, bool loop);
            GLuint* texIDs;
            int textureSize;
        
        private:
            std::mutex        mutex;
            std::thread*      thread;

            SwsContext*         img_convert_ctx;
            AVFormatContext*    pFormatCtx;
            AVStream*           videoStream2;

            AVPixelFormat       dst_pix_fmt;

            AVCodecContext*     pCodecCtx;
            AVCodec*            pCodec;


            AVFrame*            pFrame;
            AVFrame**           pFrameRGB;
            AVPacket            packet;
            uint8_t**           buffer;
            TextureManager*     textures;

            int                 i, videoStream;
            int                 frameRate = 0;
            int                 frameFinished;
            int                 numBytes;

            bool                run;
            bool				decode;
            bool                requestStop = false;

            bool				m_pause; // initialise to false in constructor!
            std::mutex 		m_pause_mutex;
            std::condition_variable m_pause_changed;

            double              internTime = 0.0;
            double              actDt = 0.0;
            double				lastTime=0.0;
        
            int                 lastReadFrame = -1;
            int                 nrFramesBuffered = 0;
            int                 readBuffer = 1;
            int                 waitReadBuf = 0;
            int                 frame_count = 0;


    };
}

#endif
