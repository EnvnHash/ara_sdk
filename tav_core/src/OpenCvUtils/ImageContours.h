/*
 * ImageContours.h
 *
 *  Created on: 29.01.2017
 *      Copyright by Sven Hahne
 */

#ifndef IMAGECONTOURS_H_
#define IMAGECONTOURS_H_

#include <iostream>
#include "GLUtils/TextureManager.h"
#include "Shaders/Shaders.h"
#include "math_utils.h"
#include "GLUtils/Noise3DTexGen.h"
#include <glm/ext.hpp>
#include "../headers/opencv_headers.h"

namespace tav
{

class ImageContours
{
public:
	typedef struct
	{
		glm::vec3 scale;
		glm::vec3 center;
		glm::vec3 transCenter;
		float blendPos = 0.f;
		glm::vec2 texTrans;
		glm::vec2 texScale;
		cv::Mat shape;
		GLuint texNr;
	} logoFrag;

	ImageContours(const char* _path, int min_thresh, int max_thresh);
	virtual ~ImageContours();
	GLint getFragTex(unsigned int _ind);
	unsigned int getNrFrags();

private:
	const char* path;

	cv::Mat img;
	cv::Point2f imgDim;
	std::vector<cv::Rect> boundRect;

	std::vector<std::vector<cv::Point> > contours;
	std::vector<std::vector<cv::Point> > contours_poly;
	std::vector<logoFrag> logoFrags;

};

}

#endif /* IMAGECONTOURS_H_ */
