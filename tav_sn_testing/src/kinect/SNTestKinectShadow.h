//
//  SNTestKinectShadow.h
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__SNTestKinectShadow__
#define __Tav_App__SNTestKinectShadow__

#pragma once

#include <stdio.h>
#include <iostream>

#include <GeoPrimitives/Quad.h>
#include <NiTE/NISkeleton.h>
#include <KinectInput/KinectInput.h>
#include <SceneNode.h>

namespace tav
{

class SNTestKinectShadow : public SceneNode
{
public:
	SNTestKinectShadow(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestKinectShadow();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};

private:
	Quad*               quad;
	GLuint              depthTexNr;
	GLuint              colorTexNr;
	GLuint              showTexNr = 0;

	KinectInput*        kin;
	ShaderCollector*	shCol;

	int                 frameNr = -1;
	int                 width;
	int                 height;
	bool                updateProc = false;
	bool                inited = false;
};

}

#endif /* defined(__Tav_App__SNTestKinectShadow__) */
