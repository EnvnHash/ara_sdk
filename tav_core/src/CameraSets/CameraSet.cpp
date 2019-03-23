//
//  CameraSet.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "CameraSet.h"

namespace tav
{
CameraSet::CameraSet(int _nrCameras, sceneData* _scd, OSCData* _osc) :
		scd(_scd), osc(_osc), nrCameras(_nrCameras), shCol(
				_scd->shaderCollector), scrWidth(_scd->screenWidth), scrHeight(
				_scd->screenHeight)
{
	//mCamModMatr = new glm::mat4[nrCameras];
	mCamViewMatr = new glm::mat4[nrCameras];
	mCamProjMatr = new glm::mat4[nrCameras];
	mCamNormMatr = new glm::mat3[nrCameras];
	lookAt = new glm::vec3[nrCameras];
	upVec = new glm::vec3[nrCameras];

	// for clearing the background
	fullScrCam = new GLMCamera();        // standard frustrum camera
	quad = new SingleQuad(-1.f, -1.f, 2.f, 2.f, 0.f, 0.f, 0.f, 1.f);

	maskQuad = new SingleQuad(-1.f, -1.f, 2.f, 2.f, 0.f, 0.f, 0.f, 1.f);

	// standard shader for the background clearing
	clearShader = shCol->getStdClear();

	f_scrWidth = static_cast<float>(scrWidth);
	f_scrHeight = static_cast<float>(scrHeight);
	clearCol = glm::vec4(0.f, 0.f, 0.f, 1.f);
}

//------------------------------------------------------------------------

void CameraSet::setupCamPar(bool _usesFbo)
{
	if (cp.multicam_modmat_src == 0) cp.multicam_modmat_src = new glm::mat4[nrCameras];
	if (cp.multicam_proj_mat4 == 0) cp.multicam_proj_mat4 = new glm::mat4[nrCameras];
	if (cp.multicam_mvp_mat4 == 0) cp.multicam_mvp_mat4 = new glm::mat4[nrCameras];

	for (auto i = 0; i < nrCameras; i++)
	{
		// mCamModMatr[i] = cam[i]->getModelMatr();
		cp.multicam_modmat_src[i] = cam[i]->getModelMatr();
		cp.multicam_proj_mat4[i] = cam[i]->getProjectionMatr();
		cp.multicam_mvp_mat4[i] = cam[i]->getMVP();

		mCamViewMatr[i] = cam[i]->getViewMatr();
		mCamProjMatr[i] = cam[i]->getProjectionMatr();
		mCamNormMatr[i] = cam[i]->getNormalMatr();
	}

	// init camPar
	cp.model_matrix = cam[0]->getModelMatrPtr();
	cp.model_matrix_mat4 = cam[0]->getModelMatr();
	cp.view_matrix = cam[0]->getViewMatrPtr();
	cp.view_matrix_mat4 = cam[0]->getViewMatr();
	cp.projection_matrix_mat4 = cam[0]->getProjectionMatr();
	cp.projection_matrix = cam[0]->getProjMatrPtr();
	cp.normal_matrix = cam[0]->getNormMatrPtr();
	cp.normal_matrix_mat3 = cam[0]->getNormalMatr();
	cp.mvp = cam[0]->getMVPPtr();
	cp.mvp_mat4 = cam[0]->getMVP();

	// float* pointer
	cp.multicam_model_matrix = &cp.multicam_modmat_src[0][0][0];
	cp.multicam_view_matrix = &mCamViewMatr[0][0][0];
	cp.multicam_projection_matrix = &mCamProjMatr[0][0][0];
	cp.multicam_normal_matrix = &mCamNormMatr[0][0][0];

	cp.viewerVec = cam[0]->getViewerVec();
	cp.nrCams = nrCameras;
	cp.roomDimen = scd->roomDim;

	cp.usesFbo = _usesFbo;
}

//------------------------------------------------------------------------

// create the Shader Prototype from a String Name
void CameraSet::addLightProto(std::string _protoName, sceneStructMode _mode)
{
	this->spf = new ShaderProtoFact(_protoName);

	spData* sd = new spData();
	sd->scrWidth = scrWidth;
	sd->scrHeight = scrHeight;
	sd->scnStructMode = _mode;
	lightProto[_protoName] = this->spf->Create(sd, shCol, scd);
	lightProto[_protoName]->setNrCams(nrCameras);

	this->initProto(lightProto[_protoName]);

	//_proto->assemble();
}

//--------------------------------------------------------------------------------

inline void CameraSet::clearScreen()
{
	glClear(GL_DEPTH_BUFFER_BIT);   // necessary, sucks performance
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);   // necessary, sucks performance

	glScissor(0, 0, scrWidth, scrHeight);
	glViewportIndexedf(0, 0.f, 0.f, f_scrWidth, f_scrHeight);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	clearCol.r = osc->backColor;
	clearCol.g = osc->backColor;
	clearCol.b = osc->backColor;
	clearCol.a = 1.f - osc->feedback;

	clearShader->begin();
	clearShader->setIdentMatrix4fv("m_pvm");
	clearShader->setUniform4fv("clearCol", &clearCol[0], 1);
	quad->draw();

	glDepthMask(GL_TRUE);
}

//--------------------------------------------------------------------------------

void CameraSet::renderTree(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	iterateNode(_scene, time, dt, ctxNr);
}

//--------------------------------------------------------------------------------

void CameraSet::iterateNode(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	std::vector<SceneNode*>* nodeList = _scene->getChildren();

	for (std::vector<SceneNode*>::iterator iter = nodeList->begin();
			iter != nodeList->end(); ++iter)
	{
		glm::mat4 newParent = _scene->_parentModelMat * _scene->_modelMat;

		// if has children iterate
		if ((*iter)->getChildren()->size() != 0)
		{
			updateOscPar((*iter), time, dt, ctxNr);	// scene wird nicht gerendert, deshalb Osc Update hier
			(*iter)->_parentModelMat = newParent;
			if ((*iter)->_active)
				iterateNode((*iter), time, dt, ctxNr); //calls this method for all the children which is Element

		}
		else
		{
			// if node is active -> render
			(*iter)->_parentModelMat = newParent;
			if ((*iter)->_active)
				render((*iter), time, dt, ctxNr);
		}
	}
}

//--------------------------------------------------------------------------------

void CameraSet::updateOscPar(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	_scene->updateOscPar(time, dt);
}

//--------------------------------------------------------------------------------

inline void CameraSet::mask()
{
	glViewportIndexedf(0, 0.f, 0.f, scrWidth, scrHeight);

	glClear(GL_DEPTH_BUFFER_BIT);   // necessary, sucks performance

	clearShader->begin();
	clearShader->setIdentMatrix4fv("m_pvm");
	clearShader->setUniform4fv("clearCol", &clearCol[0], 1);

	glDisable(GL_DEPTH_TEST);
	maskQuad->draw();
	glEnable(GL_DEPTH_TEST);

	clearShader->end();
}

//------------------------------------------------------------------------

void CameraSet::moveCamPos(float _x, float _y, float _z)
{
}

//------------------------------------------------------------------------

void CameraSet::moveCamPos(glm::vec3* _pos)
{
}

//------------------------------------------------------------------------

inline void CameraSet::sendIdentMtr(int camNr, GLuint program,
		const std::string & name)
{
	cam[camNr]->sendIdentMtr(program, name);
}

//------------------------------------------------------------------------

inline void CameraSet::sendIdentMtr3(int camNr, GLuint program,
		const std::string & name)
{
	cam[camNr]->sendIdentMtr3(program, name);
}

//------------------------------------------------------------------------

inline void CameraSet::sendMVP(int camNr, GLuint program,
		const std::string& name)
{
	cam[camNr]->sendMVP(program, name);
}

//------------------------------------------------------------------------

inline void CameraSet::sendModelM(int camNr, GLuint program,
		const std::string & name)
{
	cam[camNr]->sendModelM(program, name);
}

//------------------------------------------------------------------------

inline void CameraSet::sendViewM(int camNr, GLuint program,
		const std::string & name)
{
	cam[camNr]->sendViewM(program, name);
}

//------------------------------------------------------------------------

inline void CameraSet::sendProjM(int camNr, GLuint program,
		const std::string & name)
{
	cam[camNr]->sendProjM(program, name);
}

//------------------------------------------------------------------------

inline void CameraSet::sendViewModel(int camNr, GLuint program,
		const std::string & name)
{
	cam[camNr]->sendViewModel(program, name);
}

//------------------------------------------------------------------------

inline void CameraSet::sendViewModel3(int camNr, GLuint program,
		const std::string & name)
{
	cam[camNr]->sendViewModel3(program, name);
}

//------------------------------------------------------------------------

inline void CameraSet::sendModel3(int camNr, GLuint program,
		const std::string & name)
{
	cam[camNr]->sendModel3(program, name);
}

//------------------------------------------------------------------------

inline int CameraSet::getNrCameras()
{
	return nrCameras;
}

//------------------------------------------------------------------------

inline glm::mat4 CameraSet::getMVP(int camNr)
{
	return cam[camNr]->getMVP();
}

//------------------------------------------------------------------------

inline glm::mat4 CameraSet::getModelViewMatr(int camNr)
{
	return cam[camNr]->getModelViewMatr();
}

//------------------------------------------------------------------------

inline glm::mat4 CameraSet::getViewMatr(int camNr)
{
	return cam[camNr]->getViewMatr();
}

//------------------------------------------------------------------------

inline glm::mat4 CameraSet::getModelMatr(int camNr)
{
	return cam[camNr]->getModelMatr();
}

//------------------------------------------------------------------------

inline glm::mat4 CameraSet::getProjectionMatr(int camNr)
{
	return cam[camNr]->getProjectionMatr();
}

//------------------------------------------------------------------------

inline glm::mat3 CameraSet::getNormalMatr(int camNr)
{
	return cam[camNr]->getNormalMatr();
}

//------------------------------------------------------------------------

inline GLfloat* CameraSet::getMVPPtr(int camNr)
{
	return cam[camNr]->getMVPPtr();
}

//------------------------------------------------------------------------

inline GLfloat* CameraSet::getViewMatrPtr(int camNr)
{
	return cam[camNr]->getViewMatrPtr();
}

//------------------------------------------------------------------------

inline GLfloat* CameraSet::getModelMatrPtr(int camNr)
{
	return cam[camNr]->getModelMatrPtr();
}

//------------------------------------------------------------------------

inline GLfloat* CameraSet::getNormalMatrPtr(int camNr)
{
	return cam[camNr]->getNormMatrPtr();
}

//------------------------------------------------------------------------

inline glm::vec3 CameraSet::getLookAtPoint(int camNr)
{
	return cam[camNr]->getLookAtPoint();
}

//------------------------------------------------------------------------

inline glm::vec3 CameraSet::getViewerVec(int camNr)
{
	return cam[camNr]->getViewerVec();
}

//------------------------------------------------------------------------

inline float CameraSet::getNear(int camNr)
{
	return cam[camNr]->getNear();
}

//------------------------------------------------------------------------

inline float CameraSet::getFar(int camNr)
{
	return cam[camNr]->getNear();
}

//------------------------------------------------------------------------

inline void CameraSet::setMask(glm::vec3 _scale, glm::vec3 _trans)
{
	maskQuad->scale(_scale.x, _scale.y, _scale.z);
	maskQuad->translate(_trans.x, _trans.y, _trans.z);
}

//------------------------------------------------------------------------

inline void CameraSet::setId(int _id)
{
	id = _id;
}

//------------------------------------------------------------------------

CameraSet::~CameraSet()
{
	//delete mCamModMatr;
	delete mCamViewMatr;
	delete mCamProjMatr;
	delete mCamNormMatr;
	delete lookAt;
	delete upVec;
	delete fullScrCam;
	delete quad;
	delete clearShader;
	for (int i = 0; i < nrCameras; i++)
		delete cam[i];
	delete cam;
}

}
