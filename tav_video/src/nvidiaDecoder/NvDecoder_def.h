#pragma once

#ifdef NVDECODE_API_EXPORTS
#undef NVDECODE_API_EXPORTS
#define NVDECODE_API_EXPORTS
#else
#endif

#ifdef NVDECODE_API
#undef NVDECODE_API
#endif

#ifdef NVDECODE_API_EXPORTS
	#define NVDECODE_API __declspec(dllexport)
#else
	#define NVDECODE_API
#endif

#ifdef NVDECODE_API_EXPORTS_STATIC
#define NVDECODE_API_STATIC __declspec(dllexport)
#else
#define NVDECODE_API_STATIC __declspec(dllimport)
#endif

#pragma warning(disable : 4996)		// Avoid _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <iostream>
//#include <sstream>

#include <headers/gl_header.h>


/*
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#include <Windows.h>
#define DEBUG_POST(x) OutputDebugString(x)
#elif __APPLE__
#define DEBUG_POST(x) printf(x)
#elif __linux__
#define DEBUG_POST(x) printf(x)
#endif
*/

#if !defined (WIN32) && !defined (_WIN32) && !defined(WIN64) && !defined(_WIN64)
typedef unsigned char BYTE;
#define S_OK true;

#ifndef STRCASECMP
#define STRCASECMP  strcasecmp
#endif
#ifndef STRNCASECMP
#define STRNCASECMP strncasecmp
#endif

#else

#ifndef STRCASECMP
#define STRCASECMP  _stricmp
#endif
#ifndef STRNCASECMP
#define STRNCASECMP _strnicmp
#endif

#endif






//#define SSTR( x ) static_cast< std::ostringstream & >( ( std::ostringstream() << std::dec << x ) ).str()



