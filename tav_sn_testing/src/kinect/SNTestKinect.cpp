//
// SNTestKinect.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestKinect.h"

namespace tav
{

SNTestKinect::SNTestKinect(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs)
{
	kin = static_cast<KinectInput*>(_scd->kin);

	GWindowManager* winMan = static_cast<GWindowManager*>(_scd->winMan);
	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
		return this->onKey(key, scancode, action, mods); });

	quad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 0.f);
	quad->rotate(M_PI, 0.f, 0.f, 1.f);
	quad->rotate(M_PI, 0.f, 1.f, 0.f);

	shdr = scd->shaderCollector->getStdTex();

	show = 0;
}

//----------------------------------------------------

SNTestKinect::~SNTestKinect()
{
	delete quad;
}

//----------------------------------------------------

void SNTestKinect::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{

	switch (show)
	{
	case 0:
		kin->uploadColorImg();
		showTexNr = kin->getColorTexId();
		break;
	case 1:
		kin->uploadDepthImg(true);
		showTexNr = kin->getDepthTexId(true);
		break;
	case 2:
		kin->uploadIrImg();
		showTexNr = kin->getIrTexId();
		break;
	}

	glDisable(GL_CULL_FACE); // wenn enabled wird die "rueckseite" des quad nicht gezeichnet...
	glDisable(GL_DEPTH_TEST);

	shdr->begin();
	shdr->setIdentMatrix4fv("m_pvm");
	shdr->setUniform1i("tex", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, showTexNr);
	quad->draw();
	shdr->end();
}

//----------------------------------------------------

void SNTestKinect::update(double time, double dt)
{
}

//----------------------------------------------------

void SNTestKinect::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_1 :
			show = 0;
			std::cout << "show color" << std::endl;
			break;
		case GLFW_KEY_2 :
			show = 1;
			std::cout << "show depth" << std::endl;
			break;
		case GLFW_KEY_3 :
			show = 2;
			std::cout << "show ir" << std::endl;
			break;
		}
	}
}

}
