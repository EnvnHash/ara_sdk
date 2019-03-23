//
// SNKinectGeoRotate.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNKinectGeoRotate.h"

namespace tav
{
SNKinectGeoRotate::SNKinectGeoRotate(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs),
		screenScaleX(1.f), screenScaleY(1.f),
		screenTransX(0.f), screenTransY(0.f),
		pointWeight(7.7f),
		pointSize(7.f),
		kinTransY(-821.f),
		kinTransZ(1300.f),
		kinRotX(-0.28f)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	kin = static_cast<KinectInput*>(scd->kin);
	kinRepro = static_cast<KinectReproTools*>(scd->kinRepro);

//	_winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
	//	return this->onKey(key, scancode, action, mods); });

	quad = _scd->stdHFlipQuad;

	shdr = shCol->getStdTex();

	transMat = glm::mat4(1.f);
	transMat2D = glm::mat4(1.f);

	transTexId = new GLint[2];
	transTexId[0] = 0;
	transTexId[1] = 1;

	addPar("kinRotX", &kinRotX);
	addPar("kinRotY", &kinRotY);
	addPar("kinRotZ", &kinRotZ);

	addPar("kinTransX", &kinTransX);
	addPar("kinTransY", &kinTransY);
	addPar("kinTransZ", &kinTransZ);

	addPar("screenScaleX", &screenScaleX);
	addPar("screenScaleY", &screenScaleY);

	addPar("screenTransX", &screenTransX);
	addPar("screenTransY", &screenTransY);

	addPar("pointSize", &pointSize);
	addPar("pointWeight", &pointWeight);

	addPar("nearThres", &nearThres);
	addPar("farThres", &farThres);
}

//----------------------------------------------------

SNKinectGeoRotate::~SNKinectGeoRotate()
{
	delete quad;
}

//----------------------------------------------------

void SNKinectGeoRotate::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
/*

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE); // wenn enabled wird die "rueckseite" des quad nicht gezeichnet...
	glDisable(GL_DEPTH_TEST);

	shdr->begin();
	shdr->setIdentMatrix4fv("m_pvm");
	shdr->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, transTexId[1]);
//	glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));

	quad->draw();

	shdr->end();
	*/
}

//----------------------------------------------------

void SNKinectGeoRotate::update(double time, double dt)
{
	transMat = glm::rotate(glm::mat4(1.f), kinRotX, glm::vec3(1.f, 0.f, 0.f))
			* glm::rotate(glm::mat4(1.f), kinRotY, glm::vec3(0.f, 1.f, 0.f))
			* glm::rotate(glm::mat4(1.f), kinRotZ, glm::vec3(0.f, 0.f, 1.f));

	transMat *= glm::translate( glm::vec3( kinTransX, kinTransY, kinTransZ ) );

	transMat2D =  glm::translate( glm::vec3( screenTransX, screenTransY, 0.f ) )
		* glm::scale(glm::vec3(screenScaleX, screenScaleY, 1.f));

	kin->uploadDepthImg(false);
	transTexId = kinRepro->transformDepth( 0, false, &transMat[0][0], &transMat2D[0][0],
			pointSize, pointWeight, nearThres, farThres);
}

//----------------------------------------------------

void SNKinectGeoRotate::onKey(int key, int scancode, int action, int mods)
{}
}
