//
// SNTestKinectDispMap.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <cmath>

#include <headers/opencv_headers.h>
#include <GeoPrimitives/Quad.h>
#include <GLUtils/TextureManager.h>
#include <GLUtils/FBO.h>
#include <KinectInput/KinectInput.h>
#include <StereoMatching/StereoMatchingGPU.h>
#include <Shaders/Shaders.h>
#include <SceneNode.h>

namespace tav
{

class SNTestKinectDispMap : public SceneNode
{
public:
	SNTestKinectDispMap(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestKinectDispMap();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);
	void pathToResources();

private:
	KinectInput*        kin;
	Quad**              quad;
	ShaderCollector*	shCol;
	Shaders*            shdr;
	GLuint              depthTexNr;
	GLuint              colorTexNr;
	GLuint              showTexNr;
	int*                frameNr;
	int                 show;
	int                 nrDevices=1;
	int                 nrQuads;
	bool                isInit;
	bool                useIr;

	StereoMatchingGPU*  stm;
	cv::Mat*            img;
	cv::Mat*            gray;
	cv::Mat*            rimg;


	cv::Mat             imgDisparity16S;
	cv::Mat            	imgDisparity8U;
	cv::Mat             imgDisparity8UMat;

	cv::Mat             cameraMatrix[2];
	cv::Mat             distCoeffs[2];
	cv::Mat             R, T, R1, R2, P1, P2, Q;
	cv::UMat            uCameraMatrix[2];
	cv::UMat            uDistCoeffs[2];
	cv::UMat            uR, uT, uR1, uR2, uP1, uP2, uQ;
	cv::Rect            validRoi[2];
	cv::Size            imageSize;

	cv::UMat            rmap[2][2];
	cv::Ptr<cv::StereoSGBM> sgbm;

	int                 p1, p2;
	int                 speckleRange;
	int                 disp12MaxDiff;
	int                 blockSize;

	TextureManager*     dispTex;
	TextureManager**    texImg;

	int                 minDispar = 0;
	int                 numberOfDisparities = 0;
	int                 preFilterCap = 0;
	int                 uniquenessRatio = 0;
	int                 speckleWindowSize = 0;

	double              lastime;
	double              updtInt;
};
}
