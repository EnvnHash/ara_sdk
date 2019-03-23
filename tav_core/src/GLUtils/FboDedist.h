//
//  FboDedist.hpp
//  tav_core
//
//  Created by Sven Hahne on 22/12/15.
//  Copyright © 2015 Sven Hahne. All rights reserved..
//

#ifndef FboDedist_hpp
#define FboDedist_hpp

#pragma once

#include <stdio.h>
#include <unistd.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "GLUtils/FBO.h"
#include "GLUtils/GWindowManager.h"
#include "Shaders/Shaders.h"
#include "GeoPrimitives/QuadArray.h"
#include "GLUtils/UniformBlock.h"
#include "headers/sceneData.h"

namespace tav
{
class FboDedist
{
public:
	typedef struct
	{
		GWindow* win;
		unsigned short width;
		unsigned short height;
		unsigned short offsX;
		unsigned short offsY;
		unsigned short cornerOffs;
		glm::vec2 cornerSize;
		FBO* fbo;
		glm::mat4 perspMat;
		glm::mat4 invPerspMat;
	} fboScreenData;

	FboDedist(sceneData* _scd, GWindowManager* _winMan);
	~FboDedist();
	void add(GWindow* _win, unsigned short width, unsigned short height,
			unsigned short offsX, unsigned short offsY);
	void init();
	void onKey(int key, int scancode, int action, int mods);
	void mouseBut(GLFWwindow* window, int button, int action, int mods);
	void mouseCursor(GLFWwindow* window, double xpos, double ypos);
	void calcMidAndDist(std::vector<fboScreenData*>::iterator it);
	void bindFbo(unsigned short _ind);
	void drawFbo(unsigned short _ind);
	void drawExtFbo(unsigned short _ind, int scrWidth, int scrHeight);

	void getPerspTrans(std::vector<fboScreenData*>::iterator it);
	std::vector<fboScreenData*>* getFboScreenData();
	void setupRenderFboShader();
	glm::mat4 cvMat33ToGlm(cv::Mat& _mat);
	void loadCalib();
	void saveCalib();

	inline void setCalibFileName(std::string _name)
	{
		fileName = _name;
	}

	inline void unbindActFbo()
	{
		if (actBoundFbo != -1)
			scrData[actBoundFbo]->fbo->unbind();
	}

private:
	sceneData* scd;
	ShaderCollector* shCol;
	Shaders* texShader;
	Shaders* colShader;
	GWindowManager* winMan;
	QuadArray* fboQuad;
	Quad* cornerQuad;
	UniformBlock* uBlock;

	std::vector<fboScreenData*> scrData;
	std::vector<glm::vec2> corners;
	std::vector<std::string> cornersNames;
	std::vector<glm::vec2> blCorners;
	std::vector<bool> cornersState;
	std::string fileName;

	std::vector<cv::Point2f> quadPoint;

	glm::mat4 mat;

	GLfloat quadSkel[8];

	unsigned int nrSep;
	unsigned int nrCtx;
	unsigned int cornersSize;
	int actBoundFbo;

	double mouseX;
	double mouseY;

	bool showCorners;
	bool newCornerVals;
};
}

#endif /* FboDedist_hpp */
