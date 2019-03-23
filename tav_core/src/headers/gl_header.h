//
//  gl_header.h
//  tav_gl4
//
//  Created by Sven Hahne on 05.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#define GLEW_STATIC

#include <GL/glew.h>
//#include <GL/glxew.h>

#ifdef __linux__
#ifndef __EMSCRIPTEN__
//#define GLFW_INCLUDE_GL_3
//#include <GL/gl.h>     // core profile
//#include <GL/glext.h>     // core profile
//#include <GL/glx.h>
#endif
#elif __APPLE__
#include <OpenGL/OpenGL.h>     // core profile
#include <OpenGL/gl3.h>     // core profile
#include <OpenGL/gl3ext.h>  // core profile
#endif

#include <iosfwd>
#include <stdio.h>
#include <iostream>

void getGlError();
