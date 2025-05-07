//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
