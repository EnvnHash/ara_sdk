//
//  ShaderProto.h
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//
//  Prototyp Klasse für Shader, hier gibt es den groben Bauplan
//  der noch von extern modifiziert werden kann

#pragma once

#include <iostream>
#include "headers/gl_header.h"
#include "GLUtils/glm_utils.h"
#include "Communication/OSC/OSCData.h"
#include "SceneNode.h"
#include "Shaders/Shaders.h"
#include "Shaders/ShaderCollector.h"
#include "Shaders/ShaderAssembler.h"

namespace tav
{
class ShaderProto
{
public:
	ShaderProto(spData* _sData, ShaderCollector* _shCol);
	~ShaderProto();
	virtual void defineVert(bool _multiCam) = 0;
	virtual void defineGeom(bool _multiCam) = 0;
	virtual void defineFrag(bool _multiCam) = 0;

	virtual void preRender(SceneNode* _scene, double time, double dt) = 0;
	virtual void sendPar(int nrCams, camPar& cp, OSCData* osc, double time) = 0;

	virtual void asmblMultiCamGeo();
	virtual void assemble();
	virtual void addHead2HeadEdgeBlend();
	virtual void add2RowsEdgeBlend();
	virtual void add3RowsEdgeBlend();
	virtual void addNRowsEdgeBlend(int nrCameras);

	virtual Shaders* getShader();
	virtual void setNrCams(int _nrCams);
	virtual void sendModelMatrix(camPar& cp);
	virtual void sendViewMatrix(camPar& cp);
	virtual void sendProjectionMatrix(camPar& cp);
	virtual void sendNormalMatrix(camPar& cp);
	virtual void sendOscPar(camPar& cp, OSCData* _osc);

	ShaderAssembler a;
	Shaders* shader;
	ShaderCollector* shCol;
	int nrCams;
	bool* enableShdrType;
	bool usesShadowMaps = false;
	bool hasMultiCamGeo = false;
	bool debug;

	glm::mat4* mCamModModMatr;
	glm::mat4 rotM;
	bool mCamModModInit = false;

	spData* sData;
};
}
