//
//  KinectReproTools.hpp
//  tav_tracking
//
//  Created by Sven Hahne on 24/10/15.
//  Copyright © 2015 Sven Hahne. All rights reserved..
//

#ifndef KinectReproTools_hpp
#define KinectReproTools_hpp

#pragma once

#include <stdio.h>
#include <iostream>
#include <string>
#include <cmath>
#include <boost/thread/mutex.hpp>

#include "KinectInput/KinectInput.h"
#include "KinectReproCalibData.h"

#include <GLUtils/GLSL/FastBlur.h>
#include <GLUtils/FBO.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/PropoImage.h>
#include <Shaders/ShaderCollector.h>
#include <GLUtils/TextureManager.h>
#include <Median.h>

namespace tav
{
class KinectReproTools
{
public:
	enum mode
	{
		CHECK_RAW_COLOR,
		CHECK_RAW_DEPTH,
		CALIB_ROT_WARP,
		CALIB_ROT_WIN,
		CHECK_ROT,
		CHECK_ROT_WARP,
		CHECK_ROT_WARP_LOADED,
		USE_ROT_WARP
	};

	typedef struct
	{
		glm::ivec2 cellSize;
		glm::vec2 cellStep;
		glm::ivec2 nrCells;
		glm::ivec2 texSize;
		glm::vec2 fTexSizeInv;
		glm::vec2 fTexSize;
		glm::vec2 texIndScaleF;
		glm::vec2 texIndOffs;
		glm::vec2 kinFovFact;
		unsigned int nrPartPerCell;
		unsigned int totNrCells;
		VAO* trigVao = 0;
		//FBO* fbo;
		PingPongFbo* ppBuf;
	} partTex;

	KinectReproTools(GWindowManager* _winMan, KinectInput* _kin,
			ShaderCollector* _shCol, int _scrWidth, int _scrHeight,
			std::string _dataPath, int _kinDevId0 = 0);
	KinectReproTools(GWindowManager* _winMan, KinectInput* _kin,
			ShaderCollector* _shCol, int _scrWidth, int _scrHeight,
			std::string _dataPath, int _kinDevId0, std::string _camCalibFile);
	~KinectReproTools();

	void initBasic();
	void init();
	void initTrigVao();

	void update();

	GLint* transformDepth(unsigned int devNr, bool useHisto, float* transMatPtr,
			float* screenTransMat, float pointSize, float pointWeight, float farThres = 5000.f,
			float nearThres = 10.f, int tRotate=0);
	GLint* getTransformDepthTex();
	void rotateDepth();
	void unwarpDepth();
	void procRotCalibWin();
	void procRotWarp();

	glm::mat4 calcChessRotMed(std::vector<Median<glm::vec3>*>& _realWorldCoord,
			KinectReproCalibData& kp);
	glm::vec3 getChessMidPoint(std::vector<glm::vec3>& _realWorldCoord,
			KinectReproCalibData& kp);
	cv::Mat getPerspTrans(std::vector<cv::Point2f>& pointbuf, float _chessWidth,
			float _chessHeight);
	void getKinRealWorldCoord(glm::vec3& inCoord);

	void drawCalib();
	void drawCheckColor();
	void drawCheckDepth();
	void drawCalibWin();
	void drawFoundChess();
	void drawCheckRotWarp();

	void setMode(mode _mode);
	void setThreshZDeep(float _val);
	void setThreshZNear(float _val);
	void setRotXOffs(float _val);
	void setHFlip(bool _val);
	void setCalibWinCorner(float _x, float _y);
	void setCalibDragInit(float _x, float _y);
	void dragCalibWin(float _x, float _y);
	void noUnwarp();

	GLuint getRotDepth();
	GLuint getUnwarpTex();
	GLuint getKinColTex();
	int getFrameNr();
	KinectReproCalibData* getCalibData();

	glm::mat4 cvMatToGlm(cv::Mat& _mat);
	glm::mat4 cvMat33ToGlm(cv::Mat& _mat);
	void perspUnwarp(glm::vec2& inPoint, glm::mat4& perspMatr);
	GLint getDepthTransTexId(int ind);

	void loadCalib(std::string _filename);
	void saveCalib(std::string _filename);

	void updateCalibWin();

	void setupUndistShdr();
	void setupGeoRotShdr();
	void setupCropShdr();

	bool useDepthDeDist = true;

private:
	cv::Mat view, rview;
	cv::Mat depthView, depthRView;
	cv::Mat* procView;
	cv::Mat map1, map2;
	cv::Mat thresh8; // maybe obsolete
	cv::Mat checkWarpDepth; // maybe obsolete
	cv::Mat cameraMatrix;
	cv::Mat distCoeffs;

	cv::Size imageSize;

	std::vector<cv::Point2f> pointbuf;
	std::vector<Median<glm::vec3>*> rwCoord;

	Median<glm::vec3>* depthNormal;
	Median<glm::vec3>* cbNormal;
	Median<glm::vec3>* cbTrans;

	KinectInput* kin;

	GWindowManager* winMan;
	GWindow* contrWin;

	FastBlur* fastBlur;

	ShaderCollector* shCol;
	Shaders* colShader;
	Shaders* texShader;
	Shaders* undistRotShdr;
	Shaders* cropShdr;
	Shaders* geoRotShdr;

	FBO* rotDepth;
	FBO* rotDepthMaessure;

	TextureManager* kinTex;
	TextureManager* kinHistoDepth;
	TextureManager* kinDepthTex;
	TextureManager** unwarpTex;

	PropoImage* chessBoard;

	Quad* whiteQuad;
	Quad* quad;
	Quad* rawQuad;
	Quad* cornerQuad;

	VAO* calibWin;

	partTex ptex;

	bool inited = false;
	bool foundChessb = false;
	bool useCalib = false;
	bool doUnwarp = true;
	bool calibWinScaling = false;

	short calibWinDragOriginInd;

	int colFrameNr = -1;
	int depthFrameNr = -1;
	int scrWidth;
	int scrHeight;
	int kinDevId0;
	int texPtr;
	int nrFrameBuf;
	int rotDepthMaessureSize;

	GLint maxGeoAmpPoints;

	float glChessBoardWidth;
	float glChessBoardHeight;
	float cornerQuadSize;
	float lastKinObjDist;
	float stdTouchDepth;

	uint8_t* rotDepthM;

	double lastBlurPix;

	glm::vec3 midPoint;
	glm::vec3 midPointWorld;
	glm::mat4 cornerQuadMat;
	std::vector<glm::vec3> realWorldCoord;

	glm::vec2 calibWinSizeInit;
	glm::vec2 calibWinCenterInit;
	glm::vec2 calibWinDragOrigin;

	glm::vec2 cellStep;

	std::string dataPath;

	mode actMode = CALIB_ROT_WARP;
	KinectReproCalibData kpCalib;

	boost::mutex drawMutex;
	boost::mutex saveMutex;

	GLint* outTexId;
};
}

#endif /* KinectReproTools_hpp */
