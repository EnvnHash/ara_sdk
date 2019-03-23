//
//  glm_utils.h
//  tav_gl4
//
//  Created by Sven Hahne on 10.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#ifndef __ofTAV__glm_utils__
#define __ofTAV__glm_utils__

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

#ifndef PI
#define PI       3.14159265358979323846
#endif

#ifndef TWO_PI
#define TWO_PI   6.28318530717958647693
#endif

#ifndef M_TWO_PI
#define M_TWO_PI   6.28318530717958647693
#endif

#ifndef FOUR_PI
#define FOUR_PI 12.56637061435917295385
#endif

#ifndef HALF_PI
#define HALF_PI  1.57079632679489661923
#endif

#ifndef DEG_TO_RAD
#define DEG_TO_RAD (PI/180.0)
#endif

glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest);
float perlinOct1D(float x, int octaves, float persistence);

#endif /* defined(__ofTAV__glm_utils__) */
