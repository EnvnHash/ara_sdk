//
//  PingPongFbo.h
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__PingPongFbo__
#define __Tav_App__PingPongFbo__

#pragma once

#include <stdio.h>
#include "headers/gl_header.h"
#include "GLUtils/FBO.h"

namespace tav
{
class PingPongFbo
{
public:
	PingPongFbo(ShaderCollector* _shCol, int _width, int _height, GLenum _type,
			GLenum _target, bool _depthBuf = false, int _nrAttachments = 1,
			int _mipMapLevels = 1, int _nrSamples = 1, GLenum _wrapMode = GL_REPEAT,
			bool _layered=false);
	PingPongFbo(ShaderCollector* _shCol, int _width, int _height, int _depth,
			GLenum _type, GLenum _target, bool _depthBuf, int _nrAttachments,
			int _mipMapLevels, int _nrSamples, GLenum _wrapMode, bool _layered);
	~PingPongFbo();

	void swap();
	void clear(float _alpha = 1.f);
	void clearWhite();
	void clearAlpha(float _alpha, float _col);
	FBO* operator[](int n)
	{
		return FBOs[n];
	}
	void cleanUp();
	void setMinFilter(GLenum _type);
	void setMagFilter(GLenum _type);

	GLuint getSrcTexId();
	GLuint getDstTexId();

	GLuint getSrcTexId(int _index);
	GLuint getDstTexId(int _index);

	FBO* src;       // Source       ->  Ping
	FBO* dst;       // Destination  ->  Pong

private:
	FBO** FBOs;    // Real addresses of ping/pong FBOs
	int flag;       // Integer for making a quick swap
	bool inited = false;
};
}

#endif /* defined(__Tav_App__PingPongFbo__) */
