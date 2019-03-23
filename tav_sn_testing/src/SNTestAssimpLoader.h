//
//  SNTestAssimpLoader.h
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__SNTestAssimpLoader__
#define __Tav_App__SNTestAssimpLoader__

#pragma once

#include <stdio.h>
#include <iostream>
#include <SceneNode.h>

#include <headers/gl_header.h>
#include <GLUtils/Assimp/AssimpImport.h>
#include <Shaders/ShaderCollector.h>

namespace tav
{

class SNTestAssimpLoader : public SceneNode
{
public:
	SNTestAssimpLoader(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestAssimpLoader();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);

private:
	ShaderCollector*	shCol;
	Shaders*            shdr;
	AssimpImport*       aImport;
	glm::mat4           transMatr;
	int                 frameNr = -1;
};
}


#endif /* defined(__Tav_App__SNTestAssimpLoader__) */
