//
// Created by user on 5/16/25.
//

#pragma once


#ifndef PI
#define PI 3.14159265358979323846
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef TWO_PI
#define TWO_PI 6.28318530717958647693
#endif

#ifndef M_TWO_PI
#define M_TWO_PI 6.28318530717958647693
#endif

#ifndef FOUR_PI
#define FOUR_PI 12.56637061435917295385
#endif

#ifndef HALF_PI
#define HALF_PI 1.57079632679489661923
#endif

#ifndef DEG_TO_RAD
#define DEG_TO_RAD (PI / 180.0)
#endif

#ifndef ONE_THIRD
#define ONE_THIRD (1.0f / 3.0f)
#endif

#ifndef FOUR_THIRDS
#define FOUR_THIRDS (4.0f / 3.0f)
#endif

#ifndef SEL_LAYER_COUNT
#define SEL_LAYER_COUNT 2
#endif

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#ifdef _WIN32
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif

#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif
#endif

#define GLSG_DONT_CARE (-1)