//
//  sceneData.h
//  tav_core
//
//  Created by Sven Hahne on 25/11/15.
//  Copyright © 2015 Sven Hahne. All rights reserved..
//

#ifndef sceneData_h
#define sceneData_h

#include <functional>
#include <vector>

#include "headers/gl_header.h"
#include "headers/global_vars.h"
#include "GeoPrimitives/Quad.h"
#include "Shaders/ShaderCollector.h"

#ifdef WITH_AUDIO
#include "GLUtils/AudioTexture.h"
#endif

namespace tav
{

typedef struct
{
	sceneStructMode scnStructMode;
	int id=0;
	int screenWidth;
	int screenHeight;
	int fboWidth;
	int fboHeight;
	int nrChannels;
	int frameSize;
	int monRefRate;
	int nrSamples;
	int nrSoundFileLayers;
	float bright = 0.f;
	float sceneSpeed;
	float speed;
	double monRefDt;
	int* chanMap;
	//int*                texNrs;
	GLuint* tex;
	GLuint* textures;
	glm::vec3 camPos;
	glm::vec3 camLookAt;
	glm::vec3 camUpVec;
	glm::vec3* roomDim;
	glm::vec4* colors;
	std::string setupName;
	std::string* dataPath;
	unsigned int nrTextures;
	ShaderCollector* shaderCollector;
#ifdef WITH_AUDIO
	AudioTexture* audioTex;
#endif
	TextureManager* backTex;
	Quad* stdQuad;
	Quad* stdHFlipQuad;
	std::string* backVidPath;
	std::map<std::string, glm::vec3>* mark;

	void* boundBoxer;
	void* fboViews;
	void* fnc;
	void* fnMapConf;
	void* ft_lib;
	void* ft_fonts;
	void* kin;
	void* kinMapping;
	void* kinRepro;
	unsigned int lastPllTexUpdt = 0;
	unsigned int nrVideoTex;
	unsigned int nrFnc = 0;
	void* openGlCbs;
	void* osc;
	void* oscHandler;
	void* pa;
	void* soundFilePlayer;
	void* texObjs;
	void* videoTextures;
	void* videoTextsActRange;
	void* winMan;
} sceneData;

}

#endif /* sceneData_h */
