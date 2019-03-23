//
//  KinectPointCloud.h
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__KinectPointCloud__
#define __Tav_App__KinectPointCloud__

#pragma once

#include <cstdint>
#include <cstring>
#include <stdio.h>
#include <boost/thread/mutex.hpp>
#include "headers/tav_types.h"
#include "GLUtils/GLMCamera.h"
#include "GeoPrimitives/Quad.h"
#include "GeoPrimitives/QuadArray.h"
#include <KinectInput/KinectInput.h>
#include "NiTE/NISkeleton.h"
#include "GLUtils/PingPongFbo.h"
#include "Shaders/Shaders.h"
#include "GLUtils/TextureManager.h"
#include "GLUtils/TFO.h"

#define STRINGIFY(A) #A

namespace tav
{
class KinectPointCloud
{
public:
	KinectPointCloud(KinectInput* _kin, ShaderCollector* _shCol, int _scrWidth,
			int _scrHeight);
	~KinectPointCloud();
	void proc(bool hasNewImg, Shaders* _shader = nullptr, TFO* _tfo = nullptr);
	void draw(Shaders* _shader, TFO* _tfo = nullptr);
	GLint getTexId();

	float thres = 0.f;
	float offZ = 0.f;
private:
	KinectInput* kin;
	ShaderCollector* shCol;
	TextureManager* tex;
	boost::mutex* mtx;
	Shaders* shader;
	Shaders* toSil_shader;
	Shaders* draw_shader;
	Quad* quad;
	Quad* fboQuad;
	QuadArray* pcQuadAr;
	NISkeleton* nis;
	PingPongFbo* ppFbo;
	GLMCamera* cam;

	std::string fragShader;
	std::string vertShader;

	std::string fragToSil;
	std::string geoToSil;
	std::string vertToSil;

	int width;
	int height;
	int scrWidth;
	int scrHeight;
	int downScaleFact;

	bool bIsReady = true;
	bool resTexInited = false;
};
}
#endif /* defined(__Tav_App__KinectPointCloud__) */
