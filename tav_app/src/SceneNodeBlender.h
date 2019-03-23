//
//  SceneNodeBlender.h
//  tav_gl4
//
//  Created by Sven Hahne on 26.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once


#include "SetupManagement/SetupLoader.h"

#include <CameraSets/CameraSet.h>
#include <GeoPrimitives/Quad.h>

#include <SceneNodes/SNMorphSceneNode.h>

namespace tav
{

class SceneNodeBlender
{
public:
	SceneNodeBlender(SetupLoader* _sl);
	~SceneNodeBlender();
	void blend(double time, double dt);
	void procOsc();
	void setSnapShotMode(bool _val);

	SceneNode* getFromSceneNode();
	SceneNode* getToSceneNode();
	SceneNode* getMorphSceneNode();
	SceneNode* getSceneNode0();

private:
	scBlendData* scbData;

	bool debug;
	bool isInited = false;

	int scNum1 = 0;
	int scNum2 = 0;
	int fromSource = 0;
	int toSource = 0;
	int oldFrom = 0;
	int oldTo = 0;
	int fromSourceTook;
	int maxNrSceneNodes = 0;
	float relBlend = 0.f;
	SetupLoader* sl;
	SNMorphSceneNode* morphSceneNode;
	OSCData* oscd;
	TFO** geoRecorders;
	CameraSet* cam;
	Shaders** recOnlyShaders;

	Quad* quad;
	SceneNode** scenes;
	SceneNode** oldSceneNodes;
};

}
