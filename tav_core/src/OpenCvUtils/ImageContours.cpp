/*
 * ImageContours.cpp
 *
 *  Created on: 29.01.2017
 *      Copyright by Sven Hahne
 */

#include "pch.h"
#include "ImageContours.h"

namespace tav
{

ImageContours::ImageContours(const char* _path, int min_thresh, int max_thresh) :
		path(_path)
{
	cv::Mat src_gray;
	//int thresh = 10;
	//int max_thresh = 255;
	cv::RNG rng(12345);

	img = cv::imread(_path, cv::IMREAD_COLOR);

	if (img.rows * img.cols <= 0)
	{
		std::cout << "Image " << _path << " is empty or cannot be found\n"
				<< std::endl;
	}

	imgDim = cv::Point2f(img.cols, img.rows);

	/// Convert image to gray and blur it
	cv::cvtColor(img, src_gray, cv::COLOR_BGR2GRAY);
	cv::blur(src_gray, src_gray, cv::Size(3, 3));

	cv::Mat threshold_output;
	std::vector<cv::Vec4i> hierarchy;

	/// Detect edges using Threshold
	cv::threshold(src_gray, threshold_output, min_thresh, max_thresh,
			cv::THRESH_BINARY);

	/// Find contours
	cv::findContours(threshold_output, contours, hierarchy, cv::RETR_TREE,
			cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

	/// Approximate contours to polygons + get bounding rects and circles
	std::vector<float> radius(contours.size());
	boundRect.resize(contours.size());
	std::vector<cv::Point2f> center(contours.size());
	contours_poly.resize(contours.size());

	for (std::size_t i = 0; i < contours.size(); i++)
	{
		cv::approxPolyDP(cv::Mat(contours[i]), contours_poly[i], 0, true);
		boundRect[i] = boundingRect(cv::Mat(contours_poly[i]));
		cv::minEnclosingCircle(contours_poly[i], center[i], radius[i]);

		cv::Point2f center = cv::Point2f(
				boundRect[i].x + boundRect[i].width / 2.f,
				boundRect[i].y + boundRect[i].height / 2.f);
		logoFrags.push_back(logoFrag());

		// position entspricht der verschiebung von 0|0 auf die center position, skaliert auf -1|1
		logoFrags.back().center = glm::vec3((center.x / imgDim.x) * 2.f - 1.f,
				-((center.y / imgDim.y) * 2.f - 1.f), 0.f);

		logoFrags.back().texTrans = glm::vec2(
				(center.x - boundRect[i].width / 2) / imgDim.x,
				1.0 - ((center.y + boundRect[i].height / 2) / imgDim.y));

		// groesse in koordinaten system -1 | 1, wie muss man das quad (w:2|h:2) skalieren,
		// damit es diesselbe Groesse wie der Teil des Logos hat?
		logoFrags.back().scale = glm::vec3((boundRect[i].width / imgDim.x),
				(boundRect[i].height / imgDim.y), 1.f);

		logoFrags.back().texScale = glm::vec2(boundRect[i].width / imgDim.x,
				boundRect[i].height / imgDim.y);

		// define offset pos
		logoFrags.back().transCenter = glm::vec3(frand(), frand(), 0.f);
		logoFrags.back().transCenter = glm::vec3(
				(sfrand() > 0.f ? -1.f : 1.f)
						* (logoFrags.back().transCenter.x + 1.f),
				(sfrand() > 0.f ? -1.f : 1.f)
						* (logoFrags.back().transCenter.y + 1.f), 0.f);

		// make mask, first draw into a Mat same size as the original
		cv::Mat drawing = cv::Mat::zeros(img.size(), CV_8UC3);
		cv::Scalar color(255, 255, 255);
		cv::drawContours(drawing, contours_poly, (int) i, color, CV_FILLED, 8,
				std::vector<cv::Vec4i>(), 0, cv::Point());

		// then copy the specific subpart
		cv::Mat sub = drawing(boundRect[i]);
		sub.copyTo(logoFrags.back().shape);

		// convert cv::Mat to opengl Texture
		glGenTextures(1, &logoFrags.back().texNr); // could be more then one, but for now, just one
		glBindTexture(GL_TEXTURE_2D, logoFrags.back().texNr);

		glTexStorage2D(GL_TEXTURE_2D, 1,                 // nr of mipmap levels
				GL_RGB8, boundRect[i].width, boundRect[i].height);

		// das hier super wichtig!!!
		glPixelStorei(GL_UNPACK_ALIGNMENT,
				(logoFrags.back().shape.step & 3) ? 1 : 4);
		glPixelStorei(GL_UNPACK_ROW_LENGTH,
				logoFrags.back().shape.step
						/ logoFrags.back().shape.elemSize());

		glTexSubImage2D(GL_TEXTURE_2D,             // target
				0,                          // mipmap level
				0, 0, boundRect[i].width, boundRect[i].height,
				GL_BGR,
				GL_UNSIGNED_BYTE, logoFrags.back().shape.ptr());
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

//----------------------------------------------------

unsigned int ImageContours::getNrFrags()
{
	return static_cast<unsigned int>(contours.size());
}

//----------------------------------------------------

GLint ImageContours::getFragTex(unsigned int _ind)
{
	return logoFrags.back().texNr;
}

//----------------------------------------------------

ImageContours::~ImageContours()
{
	// TODO Auto-generated destructor stub
}

} /* namespace tav */
