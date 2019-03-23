//
//  SNTestFaceLola.h
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__SNTestFaceLola__
#define __Tav_App__SNTestFaceLola__

#pragma once

#include <stdio.h>
#include <iostream>
#include <SceneNode.h>

#include <headers/gl_header.h>
#include <headers/opencv_headers.h>
#include <GLUtils/Assimp/AssimpImport.h>
#include <Shaders/Shaders.h>
#include <Shaders/ShaderCollector.h>
#include <GLUtils/TextureManager.h>
#include <Communication/OSC/OSCData.h>
#ifdef HAVE_OPENCV
#include <VideoTextureCv.h>
#endif
#include <AnimVal.h>

namespace tav
{

class SNTestFaceLola : public SceneNode
{
public:
	SNTestFaceLola(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestFaceLola();

	void init(TFO* _tfo = nullptr);
	void initStdShdr(camPar* cp);
	void initShdr(camPar* cp);
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void startThreads(double time, double dt);
	void stopThreads(double time, double dt);
	void cleanUp();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);

private:
	Shaders*            	shdr;
	Shaders*            	projTexShdr;
	Shaders*				stdShdr;
	ShaderCollector*		shCol;

	SceneNode* 				faceNode;
	OSCData*            	osc;
	TextureManager*     	faceTex;
#ifdef HAVE_OPENCV
	VideoTextureCv**   		vts;
#endif
	GLuint*					vtsId;

	AnimVal<float>*			blendTexVal;

	Quad*                   quad;
	Quad*					fondo;

	glm::mat4*           	transMatr;

	bool					inited = false;
	int                 	frameNr = -1;
	int						vtPtr = 0;
	float 					texSelect = 0.f;
	double					texBlendTime = 2.0;
};
}


#endif /* defined(__Tav_App__SNTestFaceLola__) */
