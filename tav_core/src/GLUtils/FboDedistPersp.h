//
//  FboDedistPersp.hpp
//  tav_core
//
//  Created by Sven Hahne on 22/12/15.
//  Copyright © 2015 Sven Hahne. All rights reserved..
//

#ifndef FboDedistPersp_hpp
#define FboDedistPersp_hpp

#pragma once

#include <stdio.h>
#include <map>
#include <vector>
#include <limits.h>
#include <functional>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "GLUtils/FBO.h"
#include "GLUtils/GWindowManager.h"
#include "Shaders/Shaders.h"
#include "GeoPrimitives/QuadArray.h"
#include "GLUtils/TextureManager.h"
#include "GLUtils/UniformBlock.h"
#include "headers/sceneData.h"
#include "GLUtils/GLMCamera.h"
#include "CameraSets/CameraSet.h"
#include "math_utils.h"

using namespace std::placeholders;

namespace tav
{

class FboDedistPersp
{
public:
	typedef struct
	{
		GWindow* win;
		unsigned int id;
		unsigned int glfwCtxId;
		GLMCamera* cam;
		glm::vec2 size;
		glm::vec2 offs;
		glm::vec2 cornerSize;
		glm::vec2 texOffs;	// in Px
		glm::vec2 texSize; 	// in Px
		glm::vec2 glfwCtxSize;
		QuadArray* quad;
		glm::mat4 gradQuad[4];
		glm::mat4 perspMat;
		glm::mat4 invPerspMat;
		glm::vec4 texMod;
		glm::vec2 corners[3][4];
		float blendAmt[4];
		float blendCornerOffs[4];
		bool cornersState[2][4];
		float m[2][4];
		float c[2][4];
		float fixX[2][4];
	} fboScreenData;

	typedef struct
	{
		glm::vec2 pos;
		std::vector<FboDedistPersp::fboScreenData*>::iterator it;
		short cornerInd = -1;
	} lastCornerMouseDown;

	enum cornerNames
	{
		BOTTOM = 0, RIGHT = 1, TOP = 2, LEFT = 3
	};

	FboDedistPersp(sceneData* _scd, GWindowManager* _winMan);
	FboDedistPersp(sceneData* _scd, GWindowManager* _winMan, int _fboWidth, int _fboHeight);
	~FboDedistPersp();
	void commonInit();
	void add(GWindow* _win, GLMCamera* _cam, float width, float height, float offsX, float offsY);
	void updtFboView(std::vector<fboView*>::iterator it);
	void onKey(int key, int scancode, int action, int mods);
	void mouseBut(GLFWwindow* window, int button, int action, int mods);
	void mouseCursor(GLFWwindow* window, double xpos, double ypos);
	void calcMidAndDist(std::vector<fboScreenData*>::iterator it);
	void adjustFrustrum(std::vector<fboScreenData*>::iterator it);
	void calcBlendPoints(std::vector<fboScreenData*>::iterator it);
	void bindFbo();
	void clearFbo();
	void clearFboAlpha(float alpha, float col = 0.f);
	void clearDepthFbo();
	void drawFboView(std::vector<fboScreenData*>::iterator it);
	void drawCorners(unsigned short _ind);
	void drawAllFboViews();
	void drawTestPic();

	GLuint getFboTex(unsigned short _ind);
	glm::vec2 getFboSize(unsigned short _ind);
	glm::vec2 getScreenSize(unsigned short _ind);
	glm::vec2 getScreenOffs(unsigned short _ind);
	float* getGradQuadMatPtr(unsigned short _ind, cornerNames corner);

	void getPerspTrans(std::vector<fboScreenData*>::iterator it);
	std::vector<FboDedistPersp::fboScreenData*>* getFboScreenData();
	GLuint getFbo();
	void setupRenderFboShader();
	void setupGradQuadShader();
	glm::mat4 cvMat33ToGlm(cv::Mat& _mat);
	void loadCalib();
	void saveCalib();

	void setCamSetupFunc(std::function<void()>* _camSetFunc)
	{
		camSetPtr = _camSetFunc;
	}

	inline void setCalibFileName(std::string _name)
	{
		fileName = _name;
	}

	inline void unbindFbo()
	{
		destFbo->unbind();
	}

	// nur aus kompatibilitaetsgruenden, sollte irgendwann weg
	inline void unbindActFbo()
	{
		destFbo->unbind();
	}

private:
	std::function<void()>* camSetPtr = 0;
	//CameraSet*                  context;

	sceneData* scd;
	ShaderCollector* shCol;
	Shaders* stdTexShader;
	Shaders* texShader;
	Shaders* colShader;
	Shaders* gradQuadShdr;

	TextureManager* testBild;
	FBO* destFbo;

	GWindowManager* winMan;

	Quad* quad;
	Quad** cornerQuad;
	Quad* redQuad;
	QuadArray* quadAr;

	std::vector<UniformBlock*> uBlock;

	std::vector<fboScreenData*> scrData;
	std::map<GWindow*, FBO*> ctxFbos;
	std::string cornersNames[4];
	std::string fileName;

	std::vector<cv::Point2f> quadPoint;

	glm::mat4 mat;
	glm::mat4 identMat;

	glm::vec4 noModTex;

	lastCornerMouseDown lastMouseDown;

	GLfloat quadSkel[8];

	int ctxWidth;
	int ctxHeight;
	int fboWidth;
	int fboHeight;
	float fboWidthF;
	float fboHeightF;
	glm::vec2 fboSize;

	unsigned int nrSep;
	unsigned int nrCtx;
	unsigned int cornersSize;

	GWindow* actBoundFbo;

	double mouseX;
	double mouseY;

	float xOffs;

	bool showCorners[2];
	bool* newCornerVals;
	bool* changeUBlock;
	bool inited = false;
	bool showTestPic = false;

	bool useExternFbo;
};
}

#endif /* FboDedistPersp_hpp */
