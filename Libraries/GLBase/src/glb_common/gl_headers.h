//
// Created by sven on 6/19/21.
//

#pragma once

#ifdef __ANDROID__
#ifndef ARA_USE_GLES31
#define ARA_USE_GLES31
#endif
#endif

#ifdef ARA_USE_EGL
#define EGL_EGLEXT_PROTOTYPES
#include "EGL/egl.h"
#include "EGL/eglext.h"
#endif

#ifdef ARA_USE_GLES31
#include <GLES/gl.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl3ext.h>
#endif

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <emscripten/emscripten.h>
#elif _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
// #include <windows.h>
#include <glew.h>
#include <wglew.h>
#elif __linux__

#ifndef ARA_USE_GLES31

#include <GL/glew.h>
// #include <GL/glxew.h> // includes xmd.h -> conflicting bool types with
// freeimage
#include <GL/glx.h>

#endif

#elif __APPLE__
#include <GL/glew.h>
#include <OpenGL/CGLCurrent.h>
#include <OpenGL/gl.h>
#endif

#ifdef ARA_USE_GLES31
#ifndef GL_BGRA
#define GL_BGRA GL_RGBA
#endif
#ifndef GL_BGR
#define GL_BGR GL_RGB
#endif
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER GL_CLAMP_TO_EDGE
#endif
#ifndef GL_DEPTH_CLAMP
#define GL_DEPTH_CLAMP 0
// #define GL_DEPTH_CLAMP GL_DEPTH_CLAMP_EXT
#endif
#ifndef GL_GEOMETRY_SHADER
#define GL_GEOMETRY_SHADER GL_GEOMETRY_SHADER_EXT
#endif

#endif
