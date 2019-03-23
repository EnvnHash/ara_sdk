//
// SNTestKinectGeoRotate.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestKinectGeoRotate.h"

namespace tav
{
SNTestKinectGeoRotate::SNTestKinectGeoRotate(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs), nearThres(100.f), farThres(4000.f)
{
	kin = static_cast<KinectInput*>(scd->kin);
	GWindowManager* winMan = static_cast<GWindowManager*>(scd->winMan);
	ShaderCollector* shCol = static_cast<ShaderCollector*>(scd->shaderCollector);

	quad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 0.f);
	quad->rotate(M_PI, 0.f, 0.f, 1.f);
	quad->rotate(M_PI, 0.f, 1.f, 0.f);

	shdr = shCol->getStdTex();

	calibFileName = (*scd->dataPath)+"calib_cam/testKinectGeoRotate.yml";

	kinRepro = new KinectReproTools(winMan, kin, shCol, _scd->screenWidth,
			scd->screenHeight, *(scd->dataPath), 0);

	kinRepro->noUnwarp();
	kinRepro->loadCalib(calibFileName);
	//	kinRepro->setHFlip(hFlip);

	transMat = glm::mat4(1.f);
	transMat2D = glm::mat4(1.f);

	addPar("kinRotX", &kinRotX);
	addPar("kinRotY", &kinRotY);
	addPar("kinRotZ", &kinRotZ);

	addPar("kinTransX", &kinTransX);
	addPar("kinTransY", &kinTransY);
	addPar("kinTransZ", &kinTransZ);

	addPar("pointSize", &pointSize);
	addPar("pointWeight", &pointWeight);

	addPar("nearThres", &nearThres);
	addPar("farThres", &farThres);

	transTexId = new GLint[2];
	transTexId[0] = 0;
	transTexId[1] = 0;
}

//----------------------------------------------------

void SNTestKinectGeoRotate::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE); // wenn enabled wird die "rueckseite" des quad nicht gezeichnet...
	glDisable(GL_DEPTH_TEST);

	shdr->begin();
	shdr->setIdentMatrix4fv("m_pvm");
	shdr->setUniform1i("tex", 0);

	//std::cout << "transTexId: " << transTexId << std::endl;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, transTexId[1]);
	//	glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));

	quad->draw();

	shdr->end();
}

//----------------------------------------------------

void SNTestKinectGeoRotate::update(double time, double dt)
{
	transMat = glm::rotate(glm::mat4(1.f), kinRotX, glm::vec3(1.f, 0.f, 0.f))
				* glm::rotate(glm::mat4(1.f), kinRotY, glm::vec3(0.f, 1.f, 0.f))
				* glm::rotate(glm::mat4(1.f), kinRotZ, glm::vec3(0.f, 0.f, 1.f));

	transMat *= glm::translate( glm::mat4(1.f), glm::vec3( kinTransX, kinTransY, kinTransZ ) );

	kin->uploadDepthImg(false);
	transTexId = kinRepro->transformDepth(
			0,
			false,
			&transMat[0][0],
			&transMat2D[0][0],
			pointSize,
			pointWeight);
}

//----------------------------------------------------

void SNTestKinectGeoRotate::onKey(int key, int scancode, int action, int mods)
{}

//----------------------------------------------------

SNTestKinectGeoRotate::~SNTestKinectGeoRotate()
{
	delete quad;
}

}
