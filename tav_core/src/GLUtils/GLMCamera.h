//
//  GLMCamera.h
//  ofTAV
//
//  Created by Sven Hahne on 04.07.14.
//
//  Creates a Matrix for a GLSL vertex shader.
//  expects the shader to receive uniform glm::mat4 MVP;
//
//
#ifndef __ofTAV__GLMCamera__
#define __ofTAV__GLMCamera__

#include <iostream>
#include <cstring>

#include "headers/gl_header.h"
#include "GLUtils/glm_utils.h"

namespace tav
{
class GLMCamera
{
public:
	enum GLMC_SETUP
	{
		PERSPECTIVE, FRUSTUM, ORTHO
	};
	GLMCamera();
	GLMCamera(GLMC_SETUP _setup, int _screenWidth, int _screenHeight,
			float _left, float _right, float _bottom, float _top, float _cpX =
					0.f, float _cpY = 0.f, float _cpZ = 1.f, float _laX = 0.f,
			float _laY = 0.f, float _laZ = 0.f, float _upX = 0.f, float _upY =
					1.f, float _upZ = 0.f, float _near = 1.f,
			float _far = 100.f, float _fov = 45.f);
	~GLMCamera();
	void init();
	void initMatrices(GLMC_SETUP _setup);
	void setupPerspective(bool _vFlip = true, float _fov = 60, float nearDist =
			0, float farDist = 0, int _scrWidth = 0, int _scrHeight = 0,
			const glm::vec2& _lensOffsetfloat = glm::vec2(0.0f, 0.0f));
	void setCamPos(glm::vec3 _pos);
	void setLookAt(glm::vec3 _lookAt);
	void setCamPos(float _x, float _y, float _z);
	void setLookAt(float _x, float _y, float _z);
	void setModelMatr(glm::mat4 _modelMatr);
	void setViewMatr(glm::mat4 _viewMatr);
	void setProjMatr(glm::mat4 _projMatr);
	void setFrustMult(float* _multVal);

	void sendIdentMtr(GLuint program, const std::string & name);
	void sendIdentMtr3(GLuint program, const std::string & name);
	void sendMVP(GLuint program, const std::string & name);
	void sendModelM(GLuint program, const std::string & name);
	void sendViewM(GLuint program, const std::string & name);
	void sendProjM(GLuint program, const std::string & name);
	void sendNormM(GLuint program, const std::string & name);
	void sendViewModel(GLuint program, const std::string & name);
	void sendViewModel3(GLuint program, const std::string & name);
	void sendModel3(GLuint program, const std::string & name);

	void modelTrans(float _x, float _y, float _z);
	void modelRot(float angle, float axisX, float axisY, float axisZ);

	glm::mat4 getMVP();
	glm::mat4 getModelViewMatr();
	glm::mat4 getViewMatr();
	glm::mat4 getModelMatr();
	glm::mat4 getProjectionMatr();
	glm::mat3 getNormalMatr();
	GLfloat* getMVPPtr();
	GLfloat* getViewMatrPtr();
	GLfloat* getModelMatrPtr();
	GLfloat* getProjMatrPtr();
	GLfloat* getNormMatrPtr();
	glm::vec3 getCamPos();
	glm::vec3 getViewerVec();
	glm::vec3 getLookAtPoint();
	float getLeft();
	float getRight();
	float getBottom();
	float getTop();
	float getNear();
	float getFar();

	void debug();

	int screenWidth;
	int screenHeight;

private:
	glm::mat4 identMtr = glm::mat4(1.0f);
	glm::mat3 identMtr3 = glm::mat3(1.0f);
	glm::mat4 MVP;
	glm::mat4 Model = glm::mat4(1.0f);
	glm::mat4 Projection;
	glm::mat4 View;
	glm::mat3 Normal;
	glm::mat4 projm;
	glm::mat4 viewm;
	glm::mat3 viewm3;
	glm::mat3 model3;

	glm::vec3 camPos;
	glm::vec3 camLookAt;
	glm::vec3 camUpVec;
	glm::vec3 target = glm::vec3(0.0f);

	GLint mvpLoc = -1;

	bool vFlip = false;
	bool forceAspectRatio = false;
	// bool  bDistanceSet = false;

	float fov;
	float aspectRatio;
	float near;
	float far;
	// float lastDistance = 0.0f;
	float left = -0.5f;
	float right = 0.5f;
	float bottom = -0.5f;
	float top = 0.5f;
	float frustMult[4];

	glm::vec2 lensOffset;
	glm::vec4 orientedViewport;

	GLMC_SETUP setup;
};
}

#endif /* defined(__ofTAV__GLMCamera__) */
