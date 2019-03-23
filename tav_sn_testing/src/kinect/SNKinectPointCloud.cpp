//
//  SNKinectPointCloud.cpp
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "SNKinectPointCloud.h"

namespace tav
{

SNKinectPointCloud::SNKinectPointCloud(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    				SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	kin = static_cast<KinectInput*>(scd->kin);

	quad = scd->stdQuad;
	quad->rotate(M_PI, 0.f, 0.f, 1.f);
	quad->rotate(M_PI, 0.f, 1.f, 0.f);

	shdr = shCol->getStdTex();

	show = 2;
}

//----------------------------------------------------

void SNKinectPointCloud::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	if (!inited && kin->isNisInited())
	{
		pCloud = new KinectPointCloud(kin, shCol, scd->screenWidth, scd->screenHeight);
		inited = true;

	} else
	{
		sendStdShaderInit(_shader);

		// use internal shader
		updateProc = kin->uploadDepthImg(false);

		pCloud->proc(updateProc, _shader, _tfo);
		pCloud->draw(_shader);
	}

}

//----------------------------------------------------

void SNKinectPointCloud::update(double time, double dt)
{
}

//----------------------------------------------------

void SNKinectPointCloud::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_1 :
			pCloud->offZ += 0.1f;
			std::cout << pCloud->offZ << std::endl;
			break;
		case GLFW_KEY_2 :
			pCloud->offZ -= 0.1f;
			std::cout << pCloud->offZ << std::endl;
			break;
		}
	}
}

//----------------------------------------------------

SNKinectPointCloud::~SNKinectPointCloud()
{
	delete quad;
	delete shdr;
}

}
