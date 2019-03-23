//
//  Cs1FboAspectQuad.h
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iosfwd>

#include "CameraSet.h"
#include "GLUtils/FboDedist.h"
#include "GLUtils/FBO.h"
#include "Shaders/Shaders.h"
#include "GeoPrimitives/QuadArray.h"

namespace tav
{
class Cs1FboAspectQuad: public CameraSet
{
public:
	Cs1FboAspectQuad(sceneData* _scd, OSCData* _osc, std::vector<fboView*>* _fboViews,
			GWindowManager* _winMan);
	~Cs1FboAspectQuad();
	void initProto(ShaderProto* _proto);
	void clearFbo();
	void renderTree(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	void render(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	void preDisp(double time, double dt);
	void postRender();
	void renderFbos(double time, double dt, unsigned int ctxNr);

	void initFboShader();
	void getPerspTrans(std::vector<fboView*>::iterator it);

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
