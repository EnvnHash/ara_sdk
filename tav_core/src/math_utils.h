/*
 *  math_utils.h
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 09.09.13.
 *  Copyright 2013 Sven Hahne. All rights reserved.
 *
 */

#pragma once

#include <math.h>
#include <utility>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <glm/glm.hpp>

//#include "vmath.h"
#include "headers/global_vars.h"

#ifndef MAX
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif

namespace tav
{
float interpolVal(float inInd, int arraySize, float* array);
float interpolVal2(float inInd, int arraySize, float* array);
float interpolVal(float inInd, int arraySize, std::vector<float>* array);
float distPointLine(glm::vec2 _point, glm::vec2 _lineP1, glm::vec2 _lineP2);
float getRandF(float min, float max);
void init_number_generator();
int random_number_generator();
float mapFloat(float value, float inputMin, float inputMax, float outputMin,
		float outputMax, bool clamp);
float clamp(float value, float min, float max);
void makeMatr(glm::mat4* _matr, bool* _inited, float xOffs, float yOffs,
		float zOffs, float rotX, float rotY, float rotZ, float scaleX,
		float scaleY, float scaleZ);
float frand();
float sfrand();
}
