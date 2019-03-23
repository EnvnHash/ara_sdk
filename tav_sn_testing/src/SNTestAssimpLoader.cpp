//
//  SNTestAssimpLoader.cpp
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//  parts from assimp simpleopenglexample part from assimp of implementation
//

#include "SNTestAssimpLoader.h"

using namespace std;

namespace tav
{
SNTestAssimpLoader::SNTestAssimpLoader(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);


	// init an AssimpImporter
	aImport = new AssimpImport(_scd, true);

	// 3rd argument is a lambda function as callback
	// object deletes itself after the callback function terminates
	aImport->load(((*scd->dataPath)+"models/models-nonbsd/X/dwarf.x").c_str(), this, [this](){
	});


	shdr = shCol->getStdDirLight();
}

//----------------------------------------------------

void SNTestAssimpLoader::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if(_vao)
	{
		unsigned int texCnt = 0;


		//DEBUG_POST( ("drawing scene: "+getName()+" transVec: "+glm::to_string(_modelMat)+"\n").c_str() );
		glBlendFunc(_blendSrc, _blendDst);

		/*
		if (textures.size() > 0)
		{
			for (unsigned int t = 0; t < int(textures.size()); t++)
			{
				glActiveTexture(GL_TEXTURE0 + t);
				_shader->setUniform1i("tex" + std::to_string(t), t);
				textures[t]->bind();
				texCnt++;
			}

			_shader->setUniform1i("hasTexture", 1);

		} else _shader->setUniform1i("hasTexture", 0);
		*/

		if (_cullFace)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);

		if (_depthTest)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);


		if (_drawIndexed)
			_vao->drawElements(GL_TRIANGLES);
		else
			_vao->draw(GL_TRIANGLES);
	}
}

//----------------------------------------------------

void SNTestAssimpLoader::update(double time, double dt)
{}

//----------------------------------------------------

void SNTestAssimpLoader::onKey(int key, int scancode, int action, int mods)
{}

//----------------------------------------------------

SNTestAssimpLoader::~SNTestAssimpLoader()
{
	delete shdr;
}

}
