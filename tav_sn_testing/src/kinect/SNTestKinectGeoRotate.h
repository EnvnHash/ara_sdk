//
// SNTestKinectGeoRotate.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>

#include <GeoPrimitives/Quad.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/TextureManager.h>
#include <KinectInput/KinectInput.h>
#include <KinectInput/KinectReproTools.h>
#include <Shaders/ShaderCollector.h>
#include <Shaders/Shaders.h>
#include <SceneNode.h>

namespace tav
{

class SNTestKinectGeoRotate : public SceneNode
{
public:
	SNTestKinectGeoRotate(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestKinectGeoRotate();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);

private:

	KinectInput*        kin;
	KinectReproTools*	kinRepro;
	Quad*               quad;
	Shaders*            shdr;
	ShaderCollector*	shCol;

	int                 frameNr = -1;
	GLint*				transTexId;

	float				kinRotX=0.f;
	float				kinRotY=0.f;
	float				kinRotZ=0.f;

	float				kinTransX=0.f;
	float				kinTransY=0.f;
	float				kinTransZ=0.f;

	float				pointSize=1.f;
	float				pointWeight=1.f;

	float				nearThres;
	float				farThres;


	std::string			calibFileName;
	glm::mat4 			transMat;
	glm::mat4			transMat2D;
};

}
