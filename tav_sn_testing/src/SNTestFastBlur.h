//
//  SNTestFastBlur.h
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__SNTestFastBlur__
#define __Tav_App__SNTestFastBlur__

#pragma once

#include <stdio.h>
#include <iostream>

#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include <GLUtils/TextureManager.h>
#include <GLUtils/PingPongFbo.h>
#include <Communication/OSC/OSCData.h>

#include <GLFW/glfw3.h>

namespace tav
{

class SNTestFastBlur : public SceneNode
{
public:
	SNTestFastBlur(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestFastBlur();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);

private:
	OSCData*            osc;

	Quad*               quad;
	Quad*               fboQuad;
	GLuint              depthTexNr;
	GLuint              colorTexNr;
	GLuint              showTexNr = 0;

	Shaders*            linearV;
	Shaders*            linearH;
	Shaders*            shdr;
	ShaderCollector*	shCol;

	PingPongFbo*        pp;

	int                 frameNr = -1;
	int                 blurW;
	int                 blurH;

	float               fWidth;
	float               fHeight;
	float               fAlpha;

	GLfloat*            blurOffsH;
	GLfloat*            blurOffsV;
	float               offsScale;
	float               blFdbk;

	bool                updateProc = false;
};
}


#endif /* defined(__Tav_App__SNTestFastBlur__) */
