//
//  SNKinectPointCloud.h
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__SNKinectPointCloud__
#define __Tav_App__SNKinectPointCloud__

#pragma once

#include <stdio.h>
#include <iostream>

#include <GeoPrimitives/Quad.h>
#include <GLUtils/TextureManager.h>
#include <KinectInput/KinectInput.h>
#include <KinectInput/KinectPointCloud.h>

#include <SceneNode.h>

namespace tav
{

class SNKinectPointCloud : public SceneNode
{
public:
	SNKinectPointCloud(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNKinectPointCloud();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);

private:
	Quad*               quad;
	GLuint              depthTexNr;
	GLuint              colorTexNr;
	GLuint              showTexNr;

	ShaderCollector*	shCol;
	Shaders*            shdr;

	KinectInput*        kin;
	KinectPointCloud*   pCloud;

	int                 frameNr = -1;
	int                 show;
	bool                updateProc = false;
	bool                inited = false;
};

}


#endif /* defined(__Tav_App__SNKinectPointCloud__) */
