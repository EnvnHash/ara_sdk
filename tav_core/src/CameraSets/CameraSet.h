//
//  CameraSet.h
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <string>
#include <map>

#include "GLUtils/glm_utils.h"
#include <GLFW/glfw3.h>

#include "GLUtils/FBO.h"
#include "GLUtils/GLMCamera.h"
#include "GeoPrimitives/SingleQuad.h"
#include "Communication/OSC/OSCData.h"
#include "headers/sceneData.h"

#include "ShaderProtoFact.h"
#include "SceneNode.h"

namespace tav
{
class CameraSet
{
public:
	CameraSet(int _nrCameras, sceneData* _scd, OSCData* _osc);
	~CameraSet();
	virtual void onKey(int key, int scancode, int action, int mods) = 0;
	virtual void mouseBut(GLFWwindow* window, int button, int action, int mods) = 0;
	virtual void mouseCursor(GLFWwindow* window, double xpos, double ypos) = 0;
	virtual void initProto(ShaderProto* _proto) = 0;
	virtual void clearScreen();
	virtual void clearFbo() = 0;
	virtual void render(SceneNode* _scene, double time, double dt, unsigned int ctxNr) = 0;
	virtual void renderTree(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	virtual void iterateNode(SceneNode* _scene, double time, double dt, unsigned int ctxNr);
	virtual void updateOscPar(SceneNode* _scene, double time, double dt, unsigned int ctxNr);

	virtual void preDisp(double time, double dt) = 0;
	virtual void renderFbos(double time, double dt, unsigned int ctxNr) = 0;
	virtual void postRender() = 0;
	virtual void mask();

	virtual void setupCamPar(bool _usesFbo = false);
	virtual void addLightProto(std::string _protoName, sceneStructMode _mode);

	virtual void moveCamPos(float _x, float _y, float _z);
	virtual void moveCamPos(glm::vec3* _pos);

	virtual void sendIdentMtr(int camNr, GLuint program, const std::string & name);
	virtual void sendIdentMtr3(int camNr, GLuint program, const std::string & name);
	virtual void sendMVP(int camNr, GLuint program, const std::string & name);
	virtual void sendModelM(int camNr, GLuint program, const std::string & name);
	virtual void sendViewM(int camNr, GLuint program, const std::string & name);
	virtual void sendProjM(int camNr, GLuint program, const std::string & name);
	virtual void sendViewModel(int camNr, GLuint program, const std::string & name);
	virtual void sendViewModel3(int camNr, GLuint program, const std::string & name);
	virtual void sendModel3(int camNr, GLuint program, const std::string & name);

	virtual int getNrCameras();
	virtual glm::mat4 getMVP(int camNr);
	virtual glm::mat4 getModelViewMatr(int camNr);
	virtual glm::mat4 getViewMatr(int camNr);
	virtual glm::mat4 getModelMatr(int camNr);
	virtual glm::mat4 getProjectionMatr(int camNr);
	virtual glm::mat3 getNormalMatr(int camNr);
	virtual GLfloat* getMVPPtr(int camNr);
	virtual GLfloat* getViewMatrPtr(int camNr);
	virtual GLfloat* getModelMatrPtr(int camNr);
	virtual GLfloat* getNormalMatrPtr(int camNr);
	virtual glm::vec3 getLookAtPoint(int camNr);
	virtual glm::vec3 getViewerVec(int camNr);
	virtual float getNear(int camNr);
	virtual float getFar(int camNr);

	virtual void setMask(glm::vec3 _scale, glm::vec3 _trans);
	virtual void setId(int _id);

	int id;
	int nrCameras;
	int scrWidth;
	int scrHeight;
	float f_scrWidth;
	float f_scrHeight;

	GLMCamera** cam;
	GLMCamera* fullScrCam;

	camPar cp;

	SingleQuad* quad;
	SingleQuad* maskQuad;

	Shaders* clearShader;

	OSCData* osc;
	sceneData* scd;
	ShaderCollector* shCol;
	ShaderProtoFact* spf;
	std::map<std::string, ShaderProto*> lightProto;

	glm::mat4* mCamModMatr;
	glm::mat4* mCamViewMatr;
	glm::mat4* mCamProjMatr;
	glm::mat3* mCamNormMatr;
	glm::vec3* lookAt;
	glm::vec3* upVec;

	glm::vec4 clearCol;
};
}
