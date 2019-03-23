//
// SNTrustHog.h
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
#include <GLUtils/TextureManager.h>
#include <headers/opencv_headers.h>
#include <SceneNode.h>


namespace tav
{

class SNTrustHog : public SceneNode
{
public:
	SNTrustHog(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTrustHog();

	void init(TFO* _tfo = nullptr);
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void startThreads(double time, double dt);
	void stopThreads(double time, double dt);
	void cleanUp();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};
#ifdef HAVE_CUDA
	virtual void processQueue();
#endif
	
private:
	ShaderCollector*		shCol;
#ifdef HAVE_CUDA
	Quad*                   quad;
	cv::VideoCapture*		cap;
	cv::Mat 				frame;
	TextureManager*			uploadTexture;
	bool                    isInited = false;
	bool					new_frame_ready = false;
	bool					processing = false;
	unsigned int			old_frame = 0;
	unsigned int			new_frame = 0;
	boost::mutex			mutex;

	cv::Ptr<cv::cuda::HOG> 	gpu_hog;
	std::vector<cv::String> filenames;

	cv::Mat 				img_aux, img, img_to_show;
	cv::cuda::GpuMat 		gpu_img;
	std::vector<cv::Rect> 	found;


	cv::Size win_stride;
	cv::Size win_size;
	cv::Size block_size;
	cv::Size block_stride;
	cv::Size cell_size;

	std::string src;
	std::string svm;

	bool svm_load;
	bool make_gray;
	bool resize_src;

	int width, height;

	double scale;

	bool hit_threshold_auto;

	int win_width;
	int win_stride_width, win_stride_height;
	int block_width;
	int block_stride_width, block_stride_height;
	int cell_width;
	int nbins;

	double hog_work_begin;
	double hog_work_fps;

	double work_begin;
	double work_fps;

	double zeroTime=0;

	float hogScale=1.05f;
	float nlevels=5.f;
	float gr_threshold=1.f;
	float hit_threshold=1.75f;
#endif
};
}
