/*
 * FreeTypeFont.h
 *
 *  Created on: 27.01.2017
 *      Copyright by Sven Hahne
 */

#ifndef _FREETYPEFONT_H_
#define _FREETYPEFONT_H_

#pragma once

#include <iostream>
#include <fstream>      // std::ifstream
#include <string>
#include <cassert>
#include <cmath>

#include "headers/gl_header.h"

// FreeType2 headers
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype2/ftadvanc.h>
#include <freetype2/ftsnames.h>
#include <freetype2/tttables.h>
#include <freetype2/ftoutln.h> // for FT_Outline_Decompose

// HarfBuzz headers
#include <hb.h>
#include <hb-ft.h>
#include <ucdn.h>  // Unicode Database and Normalization
#include <hb-ucdn.h>  // Unicode Database and Normalization
//#include <hb-icu.h> // Alternatively you can use hb-glib.h

namespace tav
{

class FreeTypeFont
{
public:
	FreeTypeFont(FT_Library* _ft_library, const char* _path, unsigned int _ind);

	void setPointSize(float _point_size);

	FT_Library* getFtLib();
	FT_Face* getFace();
	FT_Long getNumGlyphs();
	float getScale();
	float getPointSize();
	hb_font_t* getHbFont();
	const char* getName();
	long getFileSize();
	char* getBinary();
	char* getPath();

	char* read_binary_file(const char * _file, long* _file_size);
	virtual ~FreeTypeFont();

private:

	FT_Library* ft_library;
	FT_Error err;
	FT_Face ft_face;
	//FT_SfntName*		names;

	hb_font_t* hb_ft_font;

	long file_size = 0;
	bool didReadBinary = false;
	char* path;
	char * bin_read_buffer;
	unsigned int ind;

	float point_size = 50.f;
	float strokeWidth = 0.1f;

	std::string name;

	static const int device_hdpi = 72;
	static const int device_vdpi = 72;
};

} /* namespace tav */

#endif /* FREETYPEFONT_H_ */
