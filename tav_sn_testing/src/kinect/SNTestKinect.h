//
// SNTestKinect.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include <GLUtils/TextureManager.h>
#include <Shaders/Shaders.h>
#include <KinectInput/KinectInput.h>
#include <Shaders/ShaderCollector.h>
#include <GLUtils/GWindowManager.h>

namespace tav
{

class SNTestKinect : public SceneNode
{
public:
	SNTestKinect(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestKinect();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);

private:
	KinectInput*        kin;
	Quad*               quad;
	Shaders*            shdr;
	ShaderCollector*	shCol;
	GLuint              depthTexNr;
	GLuint              colorTexNr;
	GLuint              showTexNr;
	int                 frameNr = -1;
	int                 show;
};
}
