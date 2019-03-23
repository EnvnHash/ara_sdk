//
//  tav_modules.h
//  tav_scene
//
//  Created by Sven Hahne on 30/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef tav_scene_tav_modules_h
#define tav_scene_tav_modules_h

#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "headers/tav_types.h"


typedef struct
{
	void** fnc;
	void* ft_lib;
	void** ft_fonts;
	void* winMan;
	void* kin;
	void* kinMapping;
	void* kinRepro;
	void* oscData;
	void* oscHandler;
	void* pa;
	void** texObjs;
	void** videoTextures;
	void** videoTextsActRange;
	unsigned int lastPllTexUpdt = 0;
	unsigned int nrVideoTex;
	unsigned int nrFnc = 0;
	std::vector<void*>* fboViews;
} tavModules;

#endif
