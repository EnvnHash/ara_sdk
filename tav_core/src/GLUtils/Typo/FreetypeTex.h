//
//  FreetypeTex.h
//  tav_core
//
//  Created by Sven Hahne on 29/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <stdio.h>
#include <string>

#include <ft2build.h>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include "../../GLUtils/TextureManager.h"

#include FT_FREETYPE_H

namespace tav
{
class FreetypeTex
{
public:
	FreetypeTex(const char* path, int _fontSize);
	~FreetypeTex();
	void setText(std::string text);
	void ftDraw_bitmap(FT_Bitmap* bitmap, FT_Int x, FT_Int y);
	void bind(int texUnit = 0);

	bool ready();
	int getWidth();
	int getHeight();
	TextureManager* getTex();

private:
	bool ftOk;
	bool ft_use_kerning;

	FT_Library* ftLib;
	FT_Face* ftFace;
	FT_GlyphSlot* ftSlot;
	FT_UInt ftGlyphIndex;
	uint8_t* ftImg=0;
	glm::ivec2 ftImgSize;
	glm::ivec2 ftPen;
	int ftFontSize;

	TextureManager* ftTex=NULL;
};
}
