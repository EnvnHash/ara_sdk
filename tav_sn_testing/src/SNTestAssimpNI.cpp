//
//  SNTestAssimpNI.cpp
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//  parts from assimp simpleopenglexample part from assimp of implementation
//

#include "SNTestAssimpNI.h"

using namespace std;

namespace tav
{
SNTestAssimpNI::SNTestAssimpNI(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	AssimpImport* aImport = new AssimpImport(_scd, true);
	aImport->load(((*scd->dataPath)+"models/models/Collada/astroboy_walk.dae").c_str(), this, [this](){
		//regScenes->dumpTree();
	});

	transMatr = *aImport->getNormAndCenterMatr();
	transMatr = glm::scale(transMatr, glm::vec3(0.005f));

	shdr = shCol->getStdDirLight();
}

//----------------------------------------------------

SNTestAssimpNI::~SNTestAssimpNI()
{
	delete shdr;
}

//----------------------------------------------------

void SNTestAssimpNI::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	shdr->begin();
	//aImport->drawNormCenter(GL_TRIANGLES, shdr);
}

//----------------------------------------------------

void SNTestAssimpNI::update(double time, double dt)
{}

//----------------------------------------------------

void SNTestAssimpNI::onKey(int key, int scancode, int action, int mods)
{}


}
