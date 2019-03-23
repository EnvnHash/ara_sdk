//
//  SNAudioWaveGPU.h
//  Tav_App
//
//  Created by Sven Hahne on 21/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__SNAudioWaveGPU__
#define __Tav_App__SNAudioWaveGPU__

#include <stdio.h>
#include <cstring>

#include <SceneNode.h>

#include <Communication/OSC/OSCData.h>
#include <GeoPrimitives/Line.h>
#include <GeoPrimitives/Quad.h>
#include <GeoPrimitives/QuadArray.h>
#include <GLUtils/TextureBuffer.h>
#include <GLUtils/TextureManager.h>
#include <GLUtils/TFO.h>
#include <GLUtils/FBO.h>
#include <GLUtils/GLSL/FastBlur.h>
#include <Shaders/ShaderCollector.h>
#include <Shaders/Shaders.h>
#include <PAudio.h>


#define STRINGIFY(A) #A

namespace tav
{

class SNAudioWaveGPU: public SceneNode
{
public:
	SNAudioWaveGPU(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNAudioWaveGPU();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{
	}
	;
private:
	PAudio* pa;
	OSCData* oscData;
	ShaderCollector* shCol;
	Shaders* waveShader;
	QuadArray* quadAr;
	Quad* quad;
	TextureBuffer** pllWaves;
	TextureManager** pllTex;
	Line** lines;
	TFO* intTfo;
	FBO* recFbo;
	FastBlur* blur;

	glm::mat4* tMatrs;
	glm::mat4 rotVecMat;
	glm::vec4* chanCols;

	int lastBlock = -1;
	int nrSegmentsPerLine;
	int nrChan;

	float* pllOffs;
	float* signals;
	float ampScale;
	float posOffs;
	int nrPar;
	int nrInstances;

	std::string vs;
	std::string vs2;
	std::string gs;
	std::string fs;
};
}

#endif /* defined(__Tav_App__SNAudioWaveGPU__) */
