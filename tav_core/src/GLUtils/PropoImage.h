//
//  PropoImage.h
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__PropoImage__
#define __Tav_App__PropoImage__

#pragma once

#include <stdio.h>
#include "headers/gl_header.h"
#include "GLUtils/FBO.h"
#include "GeoPrimitives/Quad.h"
#include "GLUtils/TextureManager.h"

namespace tav
{
class PropoImage
{
public:
	enum propoImagePos
	{
		CENTER = 0,
		UPPER_LEFT = 1,
		UPPER_RIGHT = 2,
		LOWER_LEFT = 3,
		LOWER_RIGHT = 4
	};

	PropoImage();
	PropoImage(std::string fileName, int _screenW, int _screenH,
			float _logoWidth = 0.5f, propoImagePos _pos = CENTER,
			float _border = 0.f);
	~PropoImage();
	void setupQuad();
	void draw();
	void setWidth(float _newWidth);
	float getImgHeight();
	float getImgAspectRatio();
	GLint getTexId();
private:
	TextureManager* imgTex;
	Quad* imgQuad;
	propoImagePos pos;
	bool inited = false;

	float imgWidth;
	float imgHeight;
	float oldImgWidth;
	float oldImgHeight;

	float screenW;
	float screenH;

	float border;
	float imgAspectRatio;
	float screenAspectRatio;
};
}

#endif /* defined(__Tav_App__PropoImage__) */
