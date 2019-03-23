//
//  PropoImage.cpp
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "PropoImage.h"

namespace tav
{
PropoImage::PropoImage()
{
}

PropoImage::PropoImage(std::string fileName, int _screenW, int _screenH,
		float _logoWidth, propoImagePos _pos, float _border) :
		imgWidth(_logoWidth), border(_border), screenW(_screenW), screenH(
				_screenH), pos(_pos), imgQuad(0)
{
	imgTex = new TextureManager();
	imgTex->loadTexture2D(fileName, 1);

	setupQuad();
}

//------------------------------------------------------------------------------------------------

void PropoImage::setupQuad()
{
	imgAspectRatio = static_cast<float>(imgTex->getHeight())
			/ static_cast<float>(imgTex->getWidth());
	screenAspectRatio = static_cast<float>(screenH)
			/ static_cast<float>(screenW);

	imgHeight = imgWidth * imgAspectRatio / screenAspectRatio;
	glm::vec2 imgLowerLeftCorner;

	switch (pos)
	{
	case CENTER:
		imgLowerLeftCorner = glm::vec2(0.f - imgWidth * 0.5f - border,
				0.f - imgHeight * 0.5f - border);
		break;
	case UPPER_LEFT:
		imgLowerLeftCorner = glm::vec2(1.f - border, 1.f - imgHeight - border);
		break;
	case UPPER_RIGHT:
		imgLowerLeftCorner = glm::vec2(1.f - imgWidth - border,
				1.f - imgHeight - border);
		break;
	case LOWER_LEFT:
		imgLowerLeftCorner = glm::vec2(1.f - border, 1.f - border);
		break;
	case LOWER_RIGHT:
		imgLowerLeftCorner = glm::vec2(1.f - imgWidth - border, 1.f - border);
		break;
	default:
		imgLowerLeftCorner = glm::vec2(1.f - imgWidth - border,
				1.f - imgHeight - border);
		break;
	}

	// position to right upper corner for fullscreen
	if (imgQuad)
	{
		imgQuad->scale(imgWidth / oldImgWidth, imgHeight / oldImgHeight, 1.f);
	}
	else
	{
		imgQuad = new Quad(imgLowerLeftCorner.x, imgLowerLeftCorner.y, imgWidth,
				imgHeight, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f, 0.f, 0.f);
	}
}

//------------------------------------------------------------------------------------------------

void PropoImage::draw()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, imgTex->getId());
	imgQuad->draw();
}

//------------------------------------------------------------------------------------------------

void PropoImage::setWidth(float _newWidth)
{
	oldImgWidth = imgWidth;
	oldImgHeight = imgHeight;

	imgWidth = _newWidth;
	setupQuad();
}

//------------------------------------------------------------------------------------------------

float PropoImage::getImgHeight()
{
	return imgHeight;
}

//------------------------------------------------------------------------------------------------

float PropoImage::getImgAspectRatio()
{
	return imgAspectRatio;
}

//------------------------------------------------------------------------------------------------

GLint PropoImage::getTexId()
{
	return imgTex->getId();
}

//------------------------------------------------------------------------------------------------

PropoImage::~PropoImage()
{
}
}
