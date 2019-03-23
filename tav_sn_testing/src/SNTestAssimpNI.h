//
//  SNTestAssimpNI.h
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__SNTestAssimpNI__
#define __Tav_App__SNTestAssimpNI__

#pragma once

#include <stdio.h>
#include <iostream>

#include <headers/gl_header.h>
#include <GLUtils/Assimp/AssimpImport.h>
#include <SceneNode.h>

namespace tav
{

class SNTestAssimpNI : public SceneNode
{
public:
	SNTestAssimpNI(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestAssimpNI();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);

private:
	ShaderCollector*	shCol;
	Shaders*            shdr;

	glm::mat4           transMatr;
	int                 frameNr = -1;
};

}


#endif /* defined(__Tav_App__SNTestAssimpNI__) */
