//
//  GLSLOpticalFlowDepth.h
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__GLSLOpticalFlowDepth__
#define __Tav_App__GLSLOpticalFlowDepth__

#pragma once

#include <stdio.h>

#include "GeoPrimitives/Quad.h"
#include "GLUtils/PingPongFbo.h"
#include "Shaders/ShaderCollector.h"
#include "GLUtils/TextureManager.h"
#include "GLUtils/UniformBlock.h"

#define STRINGIFY(A) #A

namespace tav
{
class GLSLOpticalFlowDepth
{
public:
	GLSLOpticalFlowDepth(ShaderCollector* _shCol, int _width, int _height);
	~GLSLOpticalFlowDepth();
	void update();

	void initShaders();

	GLuint getLastTexId();
	GLuint getResTexId();
	GLuint getDiffTexId();

	void setCurTexId(GLuint _id);
	void setLastTexId(GLuint _id);
	void setMedian(float _median);
	void setBright(float _val);
	void setMaxDist(float _val);
	void setDiffAmp(float _val);

private:
	ShaderCollector* shCol;
	Shaders* flowShader;
	Shaders* texShader;
	Quad* quad;
	UniformBlock* uBlock;
	PingPongFbo* texture;

	int width;
	int height;

	GLuint srcId;
	GLuint lastSrcId;

	float lambda;
	float median;
	float bright;
	float maxDist;
	float diffAmp;
};
}

#endif /* defined(__Tav_App__GLSLOpticalFlowDepth__) */
