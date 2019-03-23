/*
 *  headers.h
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 03.10.13.
 *  Copyright 2013 Sven Hahne. All rights reserved.
 *
 */

#pragma once

extern int currNrThreads;
extern int maxNrThreads;

#define CPUCORES 8
#define NUMOSCSCENEPAR 16
#define OSCMEDSIZE 5
#define OSCMEMSIZE 40
#define NUMITERATORS 6
#define CHUNK_SIZE 1024
#define MAX_NUM_SIM_TEXS 6
#define MAX_NUM_SIM_FONTS 8
#define MAX_NUM_COL_SCENE 6
#define MAX_SEP_REC_BUFS 4

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*(x)))

#ifndef PI
#define PI       3.14159265358979323846
#endif

#ifndef TWO_PI
#define TWO_PI   6.28318530717958647693
#endif

enum sceneStructMode
{
	FLAT, BLEND, NODE
};
