//
// SNTrustHog.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTrustHog.h"

namespace tav
{
SNTrustHog::SNTrustHog(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs,"NoLight")
{
#ifdef HAVE_CUDA
  
	cap = new cv::VideoCapture("/home/sven/tav_data/movies/trust_kamera.mov"); // open the default camera
//	cap = new cv::VideoCapture("/home/sven/tav_data/movies/vtest.avi"); // open the default camera
	if(!cap->isOpened())  // check if we succeeded
	   std::cout << "couldnt open video" << std::endl;

	(*cap) >> frame; // get the first frame


	uploadTexture = new TextureManager();
	uploadTexture->allocate(cap->get(cv::CAP_PROP_FRAME_WIDTH), cap->get(cv::CAP_PROP_FRAME_HEIGHT),
			GL_RGBA8, GL_BGR, GL_TEXTURE_2D, GL_UNSIGNED_BYTE);

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f,
            nullptr, 1, true);

    svm_load = false;
    make_gray = false;
    resize_src = false;

    width = 640;
    height = 480;

    addPar("hogScale", &hogScale);
    addPar("nlevels", &nlevels);
    addPar("gr_threshold", &gr_threshold);
    addPar("hit_threshold", &hit_threshold);

    scale = 1.05;
    nlevels = 10;
    gr_threshold = 1;
    hit_threshold = 1.15;
    hit_threshold_auto = true;

    win_width = 48;
    win_stride_width = 8;
    win_stride_height = 8;

    block_width = 16;
    block_stride_width = 8;
    block_stride_height = 8;

    cell_width = 8;
    nbins = 9;

    cv::cuda::printShortCudaDeviceInfo(cv::cuda::getDevice());

    win_stride = cv::Size(win_stride_width, win_stride_height);
    win_size = cv::Size(win_width, win_width *2);
    block_size = cv::Size(block_width, block_width);
    block_stride = cv::Size(block_stride_width, block_stride_height);
    cell_size = cv::Size(cell_width, cell_width);

    gpu_hog = cv::cuda::HOG::create(win_size, block_size, block_stride, cell_size, nbins);


    if(svm_load)
    {
        std::vector<float> svm_model;
        const std::string model_file_name = svm;
        cv::FileStorage ifs(model_file_name, cv::FileStorage::READ);

        if (ifs.isOpened())
        {
            ifs["svm_detector"] >> svm_model;
        } else {
            const std::string what =
                    "could not load model for hog classifier from file: "
                    + model_file_name;
            throw std::runtime_error(what);
        }

        // check if the variables are initialized
        if (svm_model.empty()) {
            const std::string what =
                    "HoG classifier: svm model could not be loaded from file"
                    + model_file_name;
            throw std::runtime_error(what);
        }

        gpu_hog->setSVMDetector(svm_model);

    } else
    {
        // Create HOG descriptors and detectors here
        cv::Mat detector = gpu_hog->getDefaultPeopleDetector();
        gpu_hog->setSVMDetector(detector);
    }

    std::cout << "gpusvmDescriptorSize : " << gpu_hog->getDescriptorSize() << std::endl;
#endif
}

//----------------------------------------------------

void SNTrustHog::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{

	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

#ifdef HAVE_CUDA
	
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	sendStdShaderInit(_shader);

	if(!processing && new_frame != old_frame)
	{
 	   uploadTexture->bind();
 	   mutex.lock();

 	   glTexSubImage2D(GL_TEXTURE_2D,             // target
 					0,                          // First mipmap level
 					0, 0,                       // x and y offset
					img_to_show.cols,
					img_to_show.rows,
 					GL_BGRA,
 					GL_UNSIGNED_BYTE,
 					&img_to_show.data[0]);

 	   mutex.unlock();

 	   old_frame = new_frame;
	}

	useTextureUnitInd(0, uploadTexture->getId(), _shader, _tfo);
	quad->draw(_tfo);
#endif
}

//----------------------------------------------------

void SNTrustHog::update(double time, double dt)
{
#ifdef HAVE_CUDA
	nlevels = static_cast<int>(nlevels);

    if (!processing && cap->get(cv::CAP_PROP_POS_MSEC) * 0.001 < (time - zeroTime) && !processing)
    {
    	if (cap->get(cv::CAP_PROP_POS_FRAMES) > cap->get(cv::CAP_PROP_FRAME_COUNT) -3)
    	{
    		cap->set(cv::CAP_PROP_POS_FRAMES, 0);
    		zeroTime = time;
    	}

    	new_frame++;
    	processing = true;
    	boost::thread m_Thread = boost::thread(&SNTrustHog::processQueue, this);
    }
#endif
}

//----------------------------------------------------
#ifdef HAVE_CUDA
void SNTrustHog::processQueue()
{
	mutex.lock();

	(*cap) >> frame; // get the first frame

	//work_begin = time;

	// Change format of the image
	cv::cvtColor(frame, img_aux, cv::COLOR_BGR2BGRA);

	// Resize image
	if (resize_src) cv::resize(img_aux, img, cv::Size(width, height));
	else img = img_aux;
	img_to_show = img;

  //  std::cout << img_to_show.cols << ", " << img_to_show.rows << std::endl;

	found.clear();

	// Perform HOG classification
	//hog_work_begin = time;

	gpu_img.upload(img);
	gpu_hog->setNumLevels( nlevels );
//	gpu_hog->setHitThreshold( hit_threshold );
	gpu_hog->setGammaCorrection( true );
	gpu_hog->setHitThreshold( hit_threshold );
	gpu_hog->setWinStride(win_stride);
//	gpu_hog->setScaleFactor( scale );
	gpu_hog->setScaleFactor( hogScale );
//	gpu_hog->setGroupThreshold( gr_threshold );
	gpu_hog->setGroupThreshold( gr_threshold );
	gpu_hog->detectMultiScale(gpu_img, found);

	//double delta = time - hog_work_begin;
 //   double freq = getTickFrequency();
	//hog_work_fps = freq / delta;

	// Draw positive classified windows
	for (size_t i = 0; i < found.size(); i++)
	{
		cv::Rect r = found[i];
		cv::rectangle(img_to_show, r.tl(), r.br(), cv::Scalar(0, 255, 0), 3);
	}

	cv::putText(img_to_show, "Mode: GPU", cv::Point(5, 25), cv::FONT_HERSHEY_SIMPLEX, 1., cv::Scalar(255, 100, 0), 2);
	cv::putText(img_to_show, "FPS HOG: ", cv::Point(5, 65), cv::FONT_HERSHEY_SIMPLEX, 1., cv::Scalar(255, 100, 0), 2);
	cv::putText(img_to_show, "FPS total: " , cv::Point(5, 105), cv::FONT_HERSHEY_SIMPLEX, 1., cv::Scalar(255, 100, 0), 2);

    processing = false;

	mutex.unlock();
}

#endif
  
//----------------------------------------------------

SNTrustHog::~SNTrustHog()
{
#ifdef HAVE_CUDA
	delete quad;
#endif
}

}
