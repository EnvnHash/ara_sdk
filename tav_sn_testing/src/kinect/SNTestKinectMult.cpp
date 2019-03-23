//
// SNTestKinectMult.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestKinectMult.h"

namespace tav
{
SNTestKinectMult::SNTestKinectMult(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs), isInit(false)
{
	kin = static_cast<KinectInput*>(scd->kin);
	shdr = shCol->getStdTex();
	show = 2;
}

//----------------------------------------------------

void SNTestKinectMult::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (!isInit && kin->isReady())
	{
		nrDevices = kin->getNrDevices();
		frameNr = new int[nrDevices];
		quad = new Quad*[nrDevices];

		for (int i=0;i<nrDevices;i++)
		{
			quad[i] = new Quad(-1.f + (float)i * 2.f / (float)nrDevices, -1.f,
					2.f / (float)nrDevices, 2.f,
					glm::vec3(0.f, 0.f, 1.f),
					0.f, 0.f, 0.f, 1.f);
			quad[i]->rotate(M_PI, 0.f, 0.f, 1.f);
			quad[i]->rotate(M_PI, 0.f, 1.f, 0.f);
		}

		isInit = true;
	} else
	{
		shdr->begin();
		shdr->setIdentMatrix4fv("m_pvm");
		glActiveTexture(GL_TEXTURE0);

		for (int i=0;i<nrDevices;i++)
		{
			switch (show)
			{
			case 0:
				kin->uploadColorImg(i);
				glBindTexture(GL_TEXTURE_2D, kin->getColorTexId(i));
				quad[i]->draw();

				break;

			case 1:
				kin->uploadDepthImg(true, i);
				glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(true, i));
				quad[i]->draw();

				break;

			case 2:
				kin->uploadIrImg(i);
				glBindTexture(GL_TEXTURE_2D, kin->getIrTexId(i));
				quad[i]->draw();

				break;
			}
		}

		shdr->end();
	}
}

//----------------------------------------------------

void SNTestKinectMult::update(double time, double dt)
{
}

//----------------------------------------------------

void SNTestKinectMult::onKey(int key, int scancode, int action, int mods)
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

//----------------------------------------------------

SNTestKinectMult::~SNTestKinectMult()
{
	delete quad;
}

}
