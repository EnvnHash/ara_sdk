//
// SNTestVideoTexture.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once


#include <iostream>

#include <headers/opencv_headers.h>
#include <GeoPrimitives/Quad.h>
#include <GLUtils/TextureManager.h>
#ifdef HAVE_OPENCV
#include <VideoTextureCv.h>
#endif
#include <SceneNode.h>

namespace tav
{

class SNTestVideoTexture : public SceneNode
{
public:
	SNTestVideoTexture(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestVideoTexture();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};

private:
	Quad*				quad;
#ifdef HAVE_OPENCV
	VideoTextureCv*    	vt;
	cv::VideoCapture*	cap;
	cv::Mat 			frame;
#endif
	TextureManager*		uploadTexture;
	bool                isInited = false;
};

}
