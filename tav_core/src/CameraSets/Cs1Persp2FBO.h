//
//  Cs1Persp2FBO.h
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma GCC visibility push(default)

#pragma once

#include <iosfwd>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "CameraSet.h"
#include "GLUtils/FboDedist.h"
#include "GLUtils/FBO.h"
#include "Shaders/Shaders.h"
#include "GeoPrimitives/QuadArray.h"

namespace tav
{
class Cs1Persp2FBO: public CameraSet
{
public:
	Cs1Persp2FBO(sceneData* _scd, OSCData* _osc, std::vector<fboView*>* _fboViews,
			GWindowManager* _winMan);
	~Cs1Persp2FBO();
	void initProto(ShaderProto* _proto);
	void clearFbo();
	void renderTree(SceneNode* _scene, double time, double dt,
			unsigned int ctxNr);
	void render(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	void preDisp(double time, double dt);
	void postRender();
	void renderFbos(double time, double dt, unsigned int ctxNr);

	void initFboShader();
	void getPerspTrans(std::vector<fboView*>::iterator it);
	glm::mat4 cvMat33ToGlm(cv::Mat& _mat);

	// void setLightProto(string _protoName);
	void onKey(int key, int scancode, int action, int mods);
	void mouseBut(GLFWwindow* window, int button, int action, int mods);
	void mouseCursor(GLFWwindow* window, double xpos, double ypos);
private:
	FBO* renderFbo;
	Shaders* stdTex;
	Shaders* fboDrawShdr;
	int fboWidth;
	int fboHeight;
	GLMCamera* stdCam;

	std::vector<fboView*> fboViews;
	QuadArray* quadAr;
	std::vector<cv::Point2f> quadPoint;
	GLfloat quadSkel[8];

	FboDedist* fboDedist;
	GWindowManager* winMan;
};
}

#pragma GCC visibility pop
