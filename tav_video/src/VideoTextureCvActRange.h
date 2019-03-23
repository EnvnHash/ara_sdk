/*
 * VideoTextureCvActRange.h
 *
 *  Created on: 25.01.2017
 *      Copyright by Sven Hahne
 */

#ifndef _VIDEOTEXTURECVACTRANGE_H_
#define _VIDEOTEXTURECVACTRANGE_H_


#pragma once

#include <iostream>
#include "Shaders/ShaderCollector.h"
#include "Shaders/Shaders.h"
#include "VideoTextureCv.h"
#include "VideoActivityRange.h"


namespace tav
{
    class VideoTextureCvActRange
    {
    public:
    	VideoTextureCvActRange(ShaderCollector* _shCol, VideoTextureCv* _vidTex);
        virtual ~VideoTextureCvActRange();
        void updateDt(double newDt, bool startStop);
       // void update(double time, bool startStop);
        void calcQuadMat();

        glm::mat4* getTransMat();
        float* getTransMatPtr();
        float* getTransMatFlipHPtr();

        // hand through from VideoTextureCv
        unsigned int loadFrameToTexture();
        bool isPausing();
        void bindActFrame(unsigned int texUnit=0);
#ifndef HAVE_CUDA
        cv::Mat* getActMat();
#else
        cv::cuda::GpuMat* getActMat();
#endif
        unsigned int getWidth();
        unsigned int getHeight();
        int getActFrame();

    private:
        VideoActivityRange*		vidRange;
    	ShaderCollector* 		shCol;
    	VideoTextureCv* 		vt;
    	glm::mat4				transMat;
    	glm::mat4				transMatFlipH;

    	bool 					init = false;
    	unsigned int			actUplTexId = 0;
    	unsigned int			lastTexId = 0;
    };
}


#endif /* FFMPEG_VIDEOTEXTURECVACTRANGE_H_ */
