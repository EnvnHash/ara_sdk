//
// SNTrustTimeLapse.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once


#include <iostream>

#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include "VideoTextureCv.h"
#include <GLUtils/TextureManager.h>
#include <headers/opencv_headers.h>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

namespace tav
{

class SNTrustTimeLapse : public SceneNode
{
public:
	SNTrustTimeLapse(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTrustTimeLapse();

	void init(TFO* _tfo = nullptr);
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void startThreads(double time, double dt);
	void stopThreads(double time, double dt);
	void cleanUp();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};
	virtual void processQueue();

private:
	Quad*                   quad;
	Quad*                   flipQuad;
    TextureManager*     	uploadTexture;

	VideoTextureCv* 		vt;
	Shaders*            	stdTex;
    ShaderCollector*				shCol;

	cv::VideoCapture*		cap=0;
	cv::Mat 				frame;

	std::string				backVidPath;

	bool					processing=false;
	bool					hasFrame=false;
    bool					hasTex=false;

    float					alpha=0.f;
	double					zeroTime=0;
	double					step;
    unsigned int			new_frame;
    unsigned int			old_frame;
	boost::mutex			mutex;
};
}
