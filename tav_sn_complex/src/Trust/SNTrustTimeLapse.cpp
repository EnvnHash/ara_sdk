//
// SNTrustTimeLapse.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTrustTimeLapse.h"

#define STRINGIFY(A) #A


namespace tav
{
SNTrustTimeLapse::SNTrustTimeLapse(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs,"NoLight"), step(20.0)
{
	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f,
            nullptr, 1, true);

	addPar("alpha", &alpha);
    stdTex = shCol->getStdTexAlpha();

    backVidPath = (*scd->dataPath)+"/movies/trust_kamera.mov";
}

//----------------------------------------------------

void SNTrustTimeLapse::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	//

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);
	stdTex->setUniform1f("alpha", alpha);
	uploadTexture->bind(0);
	quad->draw(_tfo);

	_shader->begin();
}

//----------------------------------------------------

void SNTrustTimeLapse::update(double time, double dt)
{
	double accelTime = time * 20.0;

    if(!cap)
	{
		cap = new cv::VideoCapture( backVidPath ); // open the default camera
		if(!cap->isOpened())  // check if we succeeded
		   std::cout << "couldnt open video" << std::endl;

		uploadTexture = new TextureManager();
		uploadTexture->allocate(
				(int)cap->get(cv::CAP_PROP_FRAME_WIDTH),
				(int)cap->get(cv::CAP_PROP_FRAME_HEIGHT),
				GL_RGBA8, GL_BGRA, GL_TEXTURE_2D, GL_UNSIGNED_BYTE);
	}

	// update background video
	if (!processing)
	{
		double actFrame = cap->get(cv::CAP_PROP_POS_FRAMES);
		double newFrame = actFrame + step;
		if ( newFrame > cap->get(cv::CAP_PROP_FRAME_COUNT) ) newFrame = 0;
		cap->set(cv::CAP_PROP_POS_FRAMES, newFrame);
		boost::thread m_Thread = boost::thread(&SNTrustTimeLapse::processQueue, this);
	}

	if(hasFrame && new_frame != old_frame)
	{
	   mutex.lock();
	   uploadTexture->bind(0);
	   glTexSubImage2D(GL_TEXTURE_2D,             // target
						0,                          // First mipmap level
						0, 0,                       // x and y offset
						(int)cap->get(cv::CAP_PROP_FRAME_WIDTH),
						(int)cap->get(cv::CAP_PROP_FRAME_HEIGHT),
						GL_BGR,
						GL_UNSIGNED_BYTE,
						&frame.data[0]);

	   mutex.unlock();
	   hasTex = true;

	   old_frame = new_frame;
	}
}

//----------------------------------------------------

void SNTrustTimeLapse::processQueue()
{
	processing = true;
	mutex.lock();
	(*cap) >> frame; // get the first frame
	new_frame++;
	if(!hasFrame) hasFrame = true;
	mutex.unlock();
    processing = false;
}

//----------------------------------------------------

SNTrustTimeLapse::~SNTrustTimeLapse()
{
	delete quad;
}

}
