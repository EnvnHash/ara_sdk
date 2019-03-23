//
// SNTestKinectMult.h
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

namespace tav
{

class SNTestKinectMult : public SceneNode
{
public:
	SNTestKinectMult(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestKinectMult();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);

private:

	KinectInput*        kin;
	ShaderCollector*    shCol;
	Quad**              quad;
	Shaders*            shdr;
	GLuint              depthTexNr;
	GLuint              colorTexNr;
	GLuint              showTexNr;
	int*                frameNr;
	int                 show;
	int                 nrDevices=1;
	bool                isInit;
};
}
