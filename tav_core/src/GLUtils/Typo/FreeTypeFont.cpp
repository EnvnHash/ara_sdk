/*
 * FreeTypeFont.cpp
 *
 *  Created on: 27.01.2017
 *      Copyright by Sven Hahne
 */

#include "pch.h"
#include "GLUtils/Typo/FreeTypeFont.h"

namespace tav
{

FreeTypeFont::FreeTypeFont(FT_Library* _ft_library, const char* _path,
		unsigned int _ind) :
		ft_library(_ft_library), ind(_ind)
{
	err = FT_New_Face(*ft_library, _path, 0, &ft_face);
	assert(!err);

	name = "font" + std::to_string(ind);

	// copy path for later use
	path = new char[strlen(_path) + 1]
	{ };
	std::copy(_path, _path + strlen(_path), path);

	// Get our harfbuzz font/face structs
	hb_ft_font = hb_ft_font_create(ft_face, hb_destroy_func_t(FT_Done_Face));
}

//----------------------------------------------------

void FreeTypeFont::setPointSize(float _point_size)
{
	point_size = _point_size;
	const FT_F26Dot6 ptSize26Dot6 = FT_F26Dot6(std::round(point_size * 64));
	err = FT_Set_Char_Size(ft_face, 0, ptSize26Dot6, device_hdpi, device_vdpi);
	assert(!err);
}

//----------------------------------------------------

FT_Library* FreeTypeFont::getFtLib()
{
	return ft_library;
}

//----------------------------------------------------

FT_Face* FreeTypeFont::getFace()
{
	return &ft_face;
}

//----------------------------------------------------

FT_Long FreeTypeFont::getNumGlyphs()
{
	return ft_face->num_glyphs;
}

//----------------------------------------------------

float FreeTypeFont::getPointSize()
{
	return point_size;
}

//----------------------------------------------------

hb_font_t* FreeTypeFont::getHbFont()
{
	return hb_ft_font;
}

//----------------------------------------------------

const char* FreeTypeFont::getName()
{
	return name.c_str();
}

//----------------------------------------------------

long FreeTypeFont::getFileSize()
{
	if (!didReadBinary)
	{
		didReadBinary = true;
		read_binary_file(path, &file_size);

	}
	return file_size;
}

//----------------------------------------------------

char* FreeTypeFont::getBinary()
{
	if (!didReadBinary)
	{
		didReadBinary = true;
		read_binary_file(path, &file_size);
	}
	return bin_read_buffer;
}

//----------------------------------------------------

char* FreeTypeFont::getPath()
{
	return path;
}

//----------------------------------------------------

char* FreeTypeFont::read_binary_file(const char * _file, long* _file_size)
{
	std::ifstream is(_file, std::ifstream::binary);

	if (is)
	{
		// get length of file:
		is.seekg(0, is.end);
		*_file_size = is.tellg();
		is.seekg(0, is.beg);

		bin_read_buffer = new char[*_file_size];

		std::cout << "Reading " << *_file_size << " characters... ";
		// read data as a block:
		is.read(bin_read_buffer, *_file_size);

		if (is)
			std::cout << "all characters read successfully.";
		else
			std::cout << "error: only " << is.gcount() << " could be read";
		is.close();

		// ...buffer contains the entire file...
	}

	return bin_read_buffer;
}

//----------------------------------------------------

FreeTypeFont::~FreeTypeFont()
{
	delete ft_face;
	if (didReadBinary)
		delete bin_read_buffer;
	// This will implicitly call FT_Done_Face on each hb_ft_font element's FT_Face
	// because FT_Done_Face is each hb_font_t's hb_destroy_func_t callback
	hb_font_destroy(hb_ft_font);

}

} /* namespace tav */
