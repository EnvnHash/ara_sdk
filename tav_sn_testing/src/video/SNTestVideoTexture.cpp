//
// SNTestVideoTexture.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestVideoTexture.h"

namespace tav
{

SNTestVideoTexture::SNTestVideoTexture(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    				SceneNode(_scd, _sceneArgs,"NoLight")
{
#ifdef HAVE_OPENCV
	VideoTextureCv** vts = static_cast<VideoTextureCv**>(scd->videoTextures);
	vt = static_cast<VideoTextureCv*>(vts[0]); // 0 = vtex0, nicht [0] der Liste !!!!

	cap = new cv::VideoCapture(1); // open the default camera
	if(!cap->isOpened())  // check if we succeeded
		std::cout << "couldnt open video" << std::endl;

	(*cap) >> frame; // get the first frame

	uploadTexture = new TextureManager();
	uploadTexture->allocate(cap->get(cv::CAP_PROP_FRAME_WIDTH), cap->get(cv::CAP_PROP_FRAME_HEIGHT),
			GL_RGBA8, GL_BGR, GL_TEXTURE_2D, GL_UNSIGNED_BYTE);
#endif
	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f,
			nullptr, 1, true);
}

//----------------------------------------------------

void SNTestVideoTexture::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	sendStdShaderInit(_shader);

#ifdef HAVE_OPENCV

	if (cap->get(cv::CAP_PROP_POS_MSEC) * 0.001 < time)
	{
		uploadTexture->bind();
		glTexSubImage2D(GL_TEXTURE_2D,             // target
				0,                          // First mipmap level
				0, 0,                       // x and y offset
				cap->get(cv::CAP_PROP_FRAME_WIDTH),
				cap->get(cv::CAP_PROP_FRAME_HEIGHT),
				GL_BGR,
				GL_UNSIGNED_BYTE,
				&frame.data[0]);
		(*cap) >> frame; // get the first frame
	}

	useTextureUnitInd(0, uploadTexture->getId(), _shader, _tfo);
#endif

	quad->draw(_tfo);
}

//----------------------------------------------------

void SNTestVideoTexture::update(double time, double dt)
{
}

//----------------------------------------------------

SNTestVideoTexture::~SNTestVideoTexture()
{
#ifdef HAVE_OPENCV
	delete cap;
#endif
	delete quad;
}

}
