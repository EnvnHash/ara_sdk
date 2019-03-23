//
// SNTrustOpticalFlow.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once


#include <iostream>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <GeoPrimitives/Quad.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include "GLUtils/GLSL/GLSLTimeMedian.h"
#include <GLUtils/FBO.h>
#include <headers/opencv_headers.h>
#include <VideoTextureCvActRange.h>

#include <SceneNode.h>


namespace tav
{

class SNTrustOpticalFlow : public SceneNode
{
public:
	SNTrustOpticalFlow(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTrustOpticalFlow();

	void init(TFO* _tfo = nullptr);
    void initShdr();
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void startThreads(double time, double dt);
	void stopThreads(double time, double dt);
	void cleanUp();
	void update(double time, double dt);
#ifdef HAVE_CUDA
	virtual void processQueue();
#endif
	void onKey(int key, int scancode, int action, int mods){};

private:
	Quad*                   quad;
	Quad*                   flipQuad;
    TextureManager*     	testTex;
    VideoTextureCvActRange* vt;
    ShaderCollector*				shCol;

    Shaders*            	stdTex;
    Shaders*            	displOptFlow;
    //VAO*					ogl_buf_vao;
    GLuint*                 VAOId;
    GLSLOpticalFlow*		flow;
    GLSLTimeMedian*			tMed;
    FBO*					medFbo;

    boost::mutex       		mutex;
#ifdef HAVE_CUDA
	cv::Ptr<cv::cuda::BroxOpticalFlow> brox;
	cv::Ptr<cv::cuda::DensePyrLKOpticalFlow> lk;
	cv::Ptr<cv::cuda::FarnebackOpticalFlow> farn;
	cv::Ptr<cv::cuda::OpticalFlowDual_TVL1> tvl1;

	cv::cuda::GpuMat		d_flow;
	cv::cuda::GpuMat*		d_frame;
    cv::cuda::GpuMat* 		d_framef;
    cv::Mat 				frame;
	cv::Mat 				gray_frame;
	cv::Mat 				size_frame;
	cv::ogl::Buffer* 		ogl_buf;
#endif
	bool                    isInited = false;
	bool                    cvOptInit = false;
	bool					optProcessing = false;

	unsigned int			vidStartOffs=0;
	unsigned int			actUplTexId=0;
	unsigned int			lastTexId=0;
	unsigned int			lastOptTexId=0;
	unsigned int			downscale=2;
	unsigned int			cvOptPtr=0;

	int 					optFrame=0;
	int						optLastFrame=0;

	float					median = 15.f;
	float					flowFdbk = 0.f;
	float					alpha = 0.f;
};
}
