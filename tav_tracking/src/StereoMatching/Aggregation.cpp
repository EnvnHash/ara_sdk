//
//  Aggregation.cpp
//  tav_tracking
//
//  Created by Sven Hahne on 9/7/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "StereoMatching/Aggregation.h"

using namespace std;

namespace tav
{
Aggregation::Aggregation(TextureManager** _inTex, cv::Size* _imageSize,
		ShaderCollector* _shCol, int _colorThreshold1, int _colorThreshold2,
		int _maxLength1, int _maxLength2) :
		inTex(_inTex), imageSize(_imageSize), shCol(_shCol), colorThreshold1(
				_colorThreshold1), colorThreshold2(_colorThreshold2), maxLength1(
				_maxLength1), maxLength2(_maxLength2)
{
	compLimitsShdr = new Shaders("shaders/adAggrLimits.vert",
			"shaders/adAggrLimits.frag", true);
	compLimitsShdr->link();

	upLimits = new FBO*[2];
	downLimits = new FBO*[2];
	leftLimits = new FBO*[2];
	rightLimits = new FBO*[2];

	fullQuad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f,
			0.f, 0.f, 0.f);

	for (int i = 0; i < 2; i++)
	{
		upLimits[i] = new FBO(shCol, imageSize->width, imageSize->height,
				GL_R32F, GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false);
		downLimits[i] = new FBO(shCol, imageSize->width, imageSize->height,
				GL_R32F, GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false);
		leftLimits[i] = new FBO(shCol, imageSize->width, imageSize->height,
				GL_R32F, GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false);
		rightLimits[i] = new FBO(shCol, imageSize->width, imageSize->height,
				GL_R32F, GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false);
	}
}

void Aggregation::computeLimits()
{
	for (int i = 0; i < 2; i++)
	{
		computeLimit(upLimits[i], -1, 0, i);
		computeLimit(downLimits[i], 1, 0, i);
		computeLimit(leftLimits[i], 0, -1, i);
		computeLimit(rightLimits[i], 0, 1, i);
	}

//        printf("compute limits \n");
//        saveToFloatDisparity(upLimits[0]->getColorImg(), *imageSize, "winSize_tav/tav_upLimit_0.png", true);
//        saveToFloatDisparity(downLimits[0]->getColorImg(), *imageSize, "winSize_tav/tav_downLimit_0.png", true);
//        saveToFloatDisparity(leftLimits[0]->getColorImg(), *imageSize, "winSize_tav/tav_leftLimit_0.png", true);
//        saveToFloatDisparity(rightLimits[0]->getColorImg(), *imageSize, "winSize_tav/tav_rightLimit_0.png", true);
}

void Aggregation::computeLimit(FBO* _limFbo, int directionH, int directionW,
		int imageNo)
{
	_limFbo->bind();

	compLimitsShdr->begin();
	compLimitsShdr->setIdentMatrix4fv("m_pvm");
	compLimitsShdr->setUniform1i("tex", 0);
	compLimitsShdr->setUniform1f("dirW",
			static_cast<float>(directionW)
					/ static_cast<float>(imageSize->width));
	compLimitsShdr->setUniform1f("dirH",
			static_cast<float>(directionH)
					/ static_cast<float>(imageSize->height));
	compLimitsShdr->setUniform1f("colorThreshold1",
			static_cast<float>(colorThreshold1) / 255.f);
	compLimitsShdr->setUniform1f("colorThreshold2",
			static_cast<float>(colorThreshold2) / 255.f);
	compLimitsShdr->setUniform1i("maxLength1", maxLength1);
	compLimitsShdr->setUniform1i("maxLength2", maxLength2);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inTex[imageNo]->getId());
	fullQuad->draw();

	_limFbo->unbind();
}

void Aggregation::saveToIntDisparity(GLuint texID, cv::Size imgSize,
		std::string filename, bool stretch)
{
	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texID);

	GLint Pixels[imgSize.height * imgSize.width * sizeof(GLint)];
	memset(Pixels, 0, imgSize.height * imgSize.width);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_INT, Pixels);
	debugMat = cv::Mat(imgSize.height, imgSize.width, CV_32S, Pixels);

	cv::Mat output(imgSize, CV_8UC3);
	std::string path("/Users/useruser/Desktop/" + filename);
	int min, max;

	printf("save: %s\n", path.c_str());

	if (stretch)
	{
		min = std::numeric_limits<int>::max();
		max = 0;

		for (size_t h = 0; h < (size_t) imageSize->height; h++)
		{
			for (size_t w = 0; w < (size_t) imageSize->width; w++)
			{
				int disparity = debugMat.at<int>((int) h, (int) w);

				if (disparity < 16842750)
				{
					if (disparity < min && disparity >= 0)
						min = disparity;
					else if (disparity > max)
						max = disparity;
				}
			}
		}
		printf("min: %d max: %d \n", min, max);
	}

	for (size_t h = 0; h < (size_t) imageSize->height; h++)
	{
		for (size_t w = 0; w < (size_t) imageSize->width; w++)
		{
			cv::Vec3b color;
			int disparity = debugMat.at<int>((int) h, (int) w);
			uchar saveVal;

			if (stretch)
			{
				saveVal = (uchar)(
						(255.f / (float) (max - min))
								* (float) (disparity - min));
			}
			else
			{
				saveVal = (uchar)(
						(float) disparity
								/ (float) std::numeric_limits<int>::max()
								* (float) std::numeric_limits<uchar>::max());
			}

			color[0] = saveVal;
			color[1] = saveVal;
			color[2] = saveVal;

			output.at < cv::Vec3b > ((int) h, (int) w) = color;
		}
	}

	cv::imwrite(path, output);
}

void Aggregation::saveToFloatDisparity(GLuint texID, cv::Size imgSize,
		std::string filename, bool stretch)
{
	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texID);

	GLfloat Pixels[imgSize.height * imgSize.width * sizeof(GLfloat)];
	memset(Pixels, 0, imgSize.height * imgSize.width);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, Pixels);

	cv::Mat output(imgSize, CV_8UC3);
	std::string path("/Users/useruser/Desktop/" + filename);
	float min, max;

	if (stretch)
	{
		min = std::numeric_limits<float>::max();
		max = 0;

		for (size_t h = 0; h < (size_t) imageSize->height; h++)
		{
			for (size_t w = 0; w < (size_t) imageSize->width; w++)
			{
				float disparity = Pixels[h * imageSize->width + w];

				if (disparity < min && disparity >= 0)
					min = disparity;
				else if (disparity > max)
					max = disparity;
			}
		}
		printf("min: %f max: %f \n", min, max);
	}

	for (size_t h = 0; h < (size_t) imageSize->height; h++)
	{
		for (size_t w = 0; w < (size_t) imageSize->width; w++)
		{
			cv::Vec3b color;
			float disparity = Pixels[h * imageSize->width + w];

			uchar saveVal;

			if (stretch)
			{
				saveVal = (uchar)((disparity - min) / (max - min) * 255.f);
			}
			else
			{
				saveVal = (uchar)(
						(float) disparity
								/ (float) std::numeric_limits<float>::max()
								* (float) std::numeric_limits<uchar>::max());
			}

			color[0] = saveVal;
			color[1] = saveVal;
			color[2] = saveVal;

			output.at < cv::Vec3b > ((int) h, (int) w) = color;
		}
	}

	cv::imwrite(path, output);
}

void Aggregation::getLimits(vector<cv::Mat> &upLimits,
		vector<cv::Mat> &downLimits, vector<cv::Mat> &leftLimits,
		vector<cv::Mat> &rightLimits) const
{
//        upLimits = this->upLimits;
//        downLimits = this->downLimits;
//        leftLimits = this->leftLimits;
//        rightLimits = this->rightLimits;
}

Aggregation::~Aggregation()
{
	delete fullQuad;
	delete[] upLimits;
	delete[] downLimits;
	delete[] leftLimits;
	delete[] rightLimits;
}
}
