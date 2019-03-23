//
//  GLMCamera.cpp
//  ofTAV
//
//  Created by Sven Hahne on 04.07.14.
//
//

#include "pch.h"
#include "GLUtils/GLMCamera.h"

namespace tav
{

GLMCamera::GLMCamera() :
		setup(FRUSTUM)
{
	camPos = glm::vec3(0, 0, 1);
	camLookAt = glm::vec3(0, 0, 0);
	camUpVec = glm::vec3(0, 1, 0);

	for (int i = 0; i < 4; i++)
		frustMult[i] = 1.f;

	init();
}

GLMCamera::GLMCamera(GLMC_SETUP _setup, int _screenWidth, int _screenHeight,
		float _left, float _right, float _bottom, float _top, float _cpX,
		float _cpY, float _cpZ, float _laX, float _laY, float _laZ, float _upX,
		float _upY, float _upZ, float _near, float _far, float _fov) :
		setup(_setup), screenWidth(_screenWidth), screenHeight(_screenHeight), left(
				_left), right(_right), bottom(_bottom), top(_top), near(_near), far(
				_far)
{
	camPos = glm::vec3(_cpX, _cpY, _cpZ);
	camLookAt = glm::vec3(_laX, _laY, _laZ);
	camUpVec = glm::vec3(_upX, _upY, _upZ);

	fov = _fov / 360.f * 2.f * M_PI;

	frustMult[0] = 0.f;
	frustMult[1] = 0.f;
	frustMult[2] = 0.f;
	frustMult[3] = 0.f;

	initMatrices(setup);
}

void GLMCamera::init()
{
	switch (setup)
	{
	case PERSPECTIVE:
		// Projection matrix : 45∞ Field of View, display range : 0.1 unit <-> 100 units
		aspectRatio = static_cast<float>(screenWidth)
				/ static_cast<float>(screenHeight);
		break;
	case FRUSTUM:
		near = 1.0f;
		far = 100.0f;
		left = -0.5f;
		right = 0.5f;
		bottom = -0.5f, top = 0.5f;
		break;
	case ORTHO:
		near = 0.0f;
		far = 100.0f;
		left = -1.0f;
		right = 1.0f;
		bottom = -1.0f, top = 1.0f;
		break;
	default:
		break;
	}
	initMatrices(setup);
}

//------------------------------------------------------------------------

void GLMCamera::initMatrices(GLMC_SETUP _setup)
{
	switch (_setup)
	{
	case PERSPECTIVE:
		// fov in radians
		aspectRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
		Projection = glm::perspective(fov, aspectRatio, near, far);
		break;
	case FRUSTUM:
		Projection = glm::frustum(left + frustMult[3], right + frustMult[1],
				bottom + frustMult[0], top + frustMult[2], near, far);
		break;
	case ORTHO:
		Projection = glm::ortho(left, right, bottom, top, near, far); // In world coordinates
		break;
	default:
		break;
	}

	// Camera matrix
	View = glm::lookAt(camPos, // Camera pos
			camLookAt, // looks at
			camUpVec);

	// Our ModelViewProjection : multiplication of our 3 matrices
	MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around
	projm = Projection * View;
	viewm = View * Model;
	viewm3 = glm::mat3(viewm);
	model3 = glm::mat3(1.0f);
	Normal = glm::mat3(glm::transpose(glm::inverse(Model)));
}

//------------------------------------------------------------------------

void GLMCamera::setupPerspective(bool _vFlip, float _fov, float nearDist,
		float farDist, int _scrWidth, int _scrHeight,
		const glm::vec2& _lensOffset)
{
	fov = _fov;
	near = nearDist;
	far = farDist;
	lensOffset = _lensOffset;
	orientedViewport = glm::vec4(0, 0, _scrWidth, _scrHeight);

	float eyeX = static_cast<float>(_scrWidth) * 0.5f;
	float eyeY = static_cast<float>(_scrHeight) * 0.5f;
	//float halfFov = PI * fov / 360;

	double tan_fovy = tan(fov * 0.5 * DEG_TO_RAD);
	float dist = eyeY / tan_fovy;

	if (near == 0) near = dist / 10.0f;
	if (far == 0) far = dist * 10.0f;

	aspectRatio = static_cast<float>(_scrWidth) / static_cast<float>(_scrHeight);
	forceAspectRatio = false;

	right = tan_fovy * aspectRatio * near;
	left = -right;
	top = tan_fovy * near;
	bottom = -top;

	camPos = glm::vec3(eyeX, eyeY, dist);
	camLookAt = glm::vec3(eyeX, eyeY, 0);

	// printf("tan_fovy: %f, right: %f, left: %f, top: %f, bottom: %f zNear: %f, zFar: %f \n",tan_fovy, right,left,top,bottom,near,far);

	vFlip = _vFlip;
}

//------------------------------------------------------------------------

void GLMCamera::setCamPos(glm::vec3 _pos)
{
	camPos = _pos;
}

//------------------------------------------------------------------------

void GLMCamera::setLookAt(glm::vec3 _lookAt)
{
	camLookAt = _lookAt;
}

//------------------------------------------------------------------------

void GLMCamera::setCamPos(float _x, float _y, float _z)
{
	camPos = glm::vec3(_x, _y, _z);
}

//------------------------------------------------------------------------

void GLMCamera::setLookAt(float _x, float _y, float _z)
{
	camLookAt = glm::vec3(_x, _y, _z);
}

//------------------------------------------------------------------------
void GLMCamera::setModelMatr(glm::mat4 _modelMatr)
{
	Model = _modelMatr;
	Normal = glm::mat3(glm::transpose(glm::inverse(Model)));
	MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around
	viewm = View * Model;
	model3 = glm::mat3(Model);
}

//------------------------------------------------------------------------
void GLMCamera::setViewMatr(glm::mat4 _viewMatr)
{
	View = _viewMatr;
	MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around
	viewm = View * Model;
	model3 = glm::mat3(Model);
}

//------------------------------------------------------------------------
void GLMCamera::setProjMatr(glm::mat4 _projMatr)
{
	Projection = _projMatr;
	MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around
	viewm = View * Model;
	model3 = glm::mat3(Model);
}

//------------------------------------------------------------------------
void GLMCamera::setFrustMult(float* _multVal)
{
	for (int i = 0; i < 4; i++)
		frustMult[i] = _multVal[i];
	initMatrices(setup);
}

//------------------------------------------------------------------------
GLMCamera::~GLMCamera()
{
}

//------------------------------------------------------------------------

void GLMCamera::sendIdentMtr(GLuint program, const std::string & name)
{
	mvpLoc = glGetUniformLocation(program, name.c_str());
	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &identMtr[0][0]);
}

//------------------------------------------------------------------------

void GLMCamera::sendIdentMtr3(GLuint program, const std::string & name)
{
	mvpLoc = glGetUniformLocation(program, name.c_str());
	glUniformMatrix3fv(mvpLoc, 1, GL_FALSE, &identMtr3[0][0]);
}

//------------------------------------------------------------------------

void GLMCamera::sendMVP(GLuint program, const std::string& name)
{
	mvpLoc = glGetUniformLocation(program, name.c_str());
	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &MVP[0][0]);
}

//------------------------------------------------------------------------

void GLMCamera::sendModelM(GLuint program, const std::string & name)
{
	mvpLoc = glGetUniformLocation(program, name.c_str());
	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &Model[0][0]);
}

//------------------------------------------------------------------------

void GLMCamera::sendViewM(GLuint program, const std::string & name)
{
	mvpLoc = glGetUniformLocation(program, name.c_str());
	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &View[0][0]);
}

//------------------------------------------------------------------------

void GLMCamera::sendProjM(GLuint program, const std::string & name)
{
	mvpLoc = glGetUniformLocation(program, name.c_str());
	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &projm[0][0]);
}

//------------------------------------------------------------------------

void GLMCamera::sendNormM(GLuint program, const std::string & name)
{
	mvpLoc = glGetUniformLocation(program, name.c_str());
	glUniformMatrix3fv(mvpLoc, 1, GL_FALSE, &Normal[0][0]);
}

//------------------------------------------------------------------------

void GLMCamera::sendViewModel(GLuint program, const std::string & name)
{
	mvpLoc = glGetUniformLocation(program, name.c_str());
	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &viewm[0][0]);
}

//------------------------------------------------------------------------

void GLMCamera::sendViewModel3(GLuint program, const std::string & name)
{
	mvpLoc = glGetUniformLocation(program, name.c_str());
	glUniformMatrix3fv(mvpLoc, 1, GL_FALSE, &viewm3[0][0]);
}

//------------------------------------------------------------------------

void GLMCamera::sendModel3(GLuint program, const std::string & name)
{
	mvpLoc = glGetUniformLocation(program, name.c_str());
	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &model3[0][0]);
}

//------------------------------------------------------------------------

void GLMCamera::modelTrans(float _x, float _y, float _z)
{
	Model = glm::translate(Model, glm::vec3(_x, _y, _z));
	MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around
	viewm = View * Model;
}

//------------------------------------------------------------------------

void GLMCamera::modelRot(float angle, float x, float y, float z)
{
	Model = glm::rotate(Model, angle, glm::vec3(x, y, z));
	MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around
	viewm = View * Model;
	model3 = glm::mat3(Model);
}

//------------------------------------------------------------------------

glm::mat4 GLMCamera::getMVP()
{
	return MVP;
}

//------------------------------------------------------------------------

glm::mat4 GLMCamera::getModelViewMatr()
{
	return viewm;
}

//------------------------------------------------------------------------

glm::mat4 GLMCamera::getViewMatr()
{
	return View;
}

//------------------------------------------------------------------------

glm::mat4 GLMCamera::getModelMatr()
{
	return Model;
}

//------------------------------------------------------------------------

glm::mat4 GLMCamera::getProjectionMatr()
{
	return Projection;
}

//------------------------------------------------------------------------

glm::mat3 GLMCamera::getNormalMatr()
{
	return Normal;
}

//------------------------------------------------------------------------

GLfloat* GLMCamera::getMVPPtr()
{
	return &MVP[0][0];
}

//------------------------------------------------------------------------

GLfloat* GLMCamera::getViewMatrPtr()
{
	return &View[0][0];
}

//------------------------------------------------------------------------

GLfloat* GLMCamera::getModelMatrPtr()
{
	return &Model[0][0];
}

//------------------------------------------------------------------------

GLfloat* GLMCamera::getProjMatrPtr()
{
	return &Projection[0][0];
}

//------------------------------------------------------------------------

GLfloat* GLMCamera::getNormMatrPtr()
{
	return &Normal[0][0];
}

//------------------------------------------------------------------------

glm::vec3 GLMCamera::getCamPos()
{
	return camPos;
}

//------------------------------------------------------------------------

glm::vec3 GLMCamera::getLookAtPoint()
{
	return camLookAt;
}

//------------------------------------------------------------------------

glm::vec3 GLMCamera::getViewerVec()
{
	return camPos - camLookAt;
}

//------------------------------------------------------------------------

float GLMCamera::getLeft()
{
	return left;
}

//------------------------------------------------------------------------

float GLMCamera::getRight()
{
	return right;
}

//------------------------------------------------------------------------

float GLMCamera::getBottom()
{
	return bottom;
}

//------------------------------------------------------------------------

float GLMCamera::getTop()
{
	return top;
}

//------------------------------------------------------------------------

float GLMCamera::getNear()
{
	return near;
}

//------------------------------------------------------------------------

float GLMCamera::getFar()
{
	return far;
}

//------------------------------------------------------------------------

void GLMCamera::debug()
{
	std::cout << "modelMatrix: " << glm::to_string(Model) << std::endl;
	std::cout << "viewMatrix: " << glm::to_string(View) << std::endl;
	std::cout << "projectionMatrix: " << glm::to_string(Projection) << std::endl
			<< std::endl;
	std::cout << "camPos: " << glm::to_string(camPos) << std::endl;
	std::cout << "lookAt: " << glm::to_string(camLookAt) << std::endl;
	std::cout << "near: " << near << " far: " << far << std::endl;
}
}
