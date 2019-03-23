//
//  FreetypeTex.cpp
//  tav_core
//
//  Created by Sven Hahne on 29/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "FreetypeTex.h"

namespace tav
{
FreetypeTex::FreetypeTex(const char* path, int _fontSize) :
		ftFontSize(_fontSize), ftOk(false)
{
	ftLib = new FT_Library();
	FT_Error error = FT_Init_FreeType(ftLib);

	if (error)
		std::cerr << "an error occured during Freetype initialisation"
				<< std::endl;
	else
	{
		ftFace = new FT_Face();
		error = FT_New_Face(*ftLib, path, 0, ftFace);

		ft_use_kerning = FT_HAS_KERNING((*ftFace));

		if (error == FT_Err_Unknown_File_Format)
			std::cerr
					<< "GUIObject::setFont Error: The font file could be opened and read, but it appears that its font format is unsupported"
					<< std::endl;
		else if (error)
			std::cerr
					<< "GUIObject::setFont Error:  The font file could not be opened or read, or that it is broken..."
					<< std::endl;
		else
		{
			error = FT_Set_Pixel_Sizes(*ftFace, 0, ftFontSize * 2);

			ftSlot = &(*ftFace)->glyph;  // a small shortcut
			ftOk = true;
		}
	}
}

//--------------------------------------------------

void FreetypeTex::setText(std::string text)
{
	FT_Error error;
	FT_UInt glyph_index;
	int height = 0;
	int width = 0;
	int negYOffs = 0;
	int kernAdj = 0;
	FT_UInt previous = 0;
	std::string yProbe = "g";
	int bitShift = 6;

	if (ftOk)
	{
		ftPen.x = 0;
		ftPen.y = 0;

		// get max negative Y-Offs
		glyph_index = FT_Get_Char_Index(*ftFace, yProbe[0]);
		error = FT_Load_Glyph(*ftFace, glyph_index, FT_LOAD_RENDER);
		negYOffs = static_cast<int>((*ftSlot)->metrics.height - (*ftSlot)->metrics.horiBearingY);

		// get width and height
		for (int n = 0; n < static_cast<int>(text.size()); n++)
		{
			glyph_index = FT_Get_Char_Index(*ftFace, text[n]);

			// retrieve kerning distance and move pen position
			if (ft_use_kerning && previous && glyph_index)
			{
				FT_Vector delta;
				FT_Get_Kerning(*ftFace, previous, glyph_index,
						FT_KERNING_DEFAULT, &delta);

				ftPen.x += delta.x >> bitShift;
				ftPen.x += kernAdj;
			}

			// load glyph image into the slot (erase previous one)
			error = FT_Load_Glyph(*ftFace, glyph_index, FT_LOAD_RENDER);
			if (error)
				continue;  // ignore errors

			if ((*ftSlot)->metrics.horiBearingY > height)
				height = static_cast<int>((*ftSlot)->metrics.horiBearingY);

			// increment pen position
			ftPen.x += (*ftSlot)->advance.x >> bitShift;
			width = ftPen.x;

			// record current glyph index
			previous = glyph_index;
		}

		height = height >> bitShift;
		negYOffs = negYOffs >> bitShift;

		//std::cout << "height: " << height << std::endl;
		//std::cout << "negYOffs: " << negYOffs << std::endl;

		// gen Tex and draw
		ftImgSize = glm::ivec2((width+1)/2*2, ((height + negYOffs)+1)/2 * 2);

		//std::cout << glm::to_string(ftImgSize) << std::endl;

		if (ftImg) {
			delete ftImg;
		}

		//ftImg = new uint8_t[128];
		//memset(ftImg, 0, 128);
		ftImg = new uint8_t[ftImgSize.x * ftImgSize.y * 4];
		memset(ftImg, 0, ftImgSize.x * ftImgSize.y * 4);

		ftPen.x = 0;
		ftPen.y = 0;
		previous = 0;

		for (int n = 0; n < static_cast<int>(text.size()); n++)
		{
			glyph_index = FT_Get_Char_Index(*ftFace, text[n]);

			// retrieve kerning distance and move pen position
			if (ft_use_kerning && previous && glyph_index)
			{
				FT_Vector delta;
				FT_Get_Kerning(*ftFace, previous, glyph_index,
						FT_KERNING_DEFAULT, &delta);
				ftPen.x += delta.x >> bitShift;
				ftPen.x += kernAdj;
			}

			// load glyph image into the slot (erase previous one)
			error = FT_Load_Glyph(*ftFace, glyph_index, FT_LOAD_RENDER);
			if (error)
				continue; // ignore errors

			ftDraw_bitmap(&(*ftSlot)->bitmap, ftPen.x + (*ftSlot)->bitmap_left, height - (*ftSlot)->bitmap_top);

			// increment pen position
			ftPen.x += (*ftSlot)->advance.x >> bitShift;
			width = ftPen.x;

			// record current glyph index
			previous = glyph_index;
		}

		/*
		// upload tex
		if (ftTex) delete ftTex;

		ftTex = new TextureManager();
		ftTex->allocate(ftImgSize.x, ftImgSize.y, GL_RGBA8, GL_RGBA, GL_TEXTURE_2D);
		ftTex->setWraping(GL_CLAMP_TO_EDGE);

		ftTex->bind(0);
		glTexSubImage2D(GL_TEXTURE_2D,             // target
				0,                          // First mipmap level
				0, 0,                       // x and y offset
				ftImgSize.x, ftImgSize.y, GL_BGRA, GL_UNSIGNED_BYTE, ftImg);
					*/
	} else {
		printf("FT not ok \n");
	}
}

//--------------------------------------------------

void FreetypeTex::ftDraw_bitmap(FT_Bitmap* bitmap, FT_Int x, FT_Int y)
{
	if (ftImg)
	{
		FT_Int i, j, p, q;
		FT_Int x_max = x + bitmap->width;
		FT_Int y_max = y + bitmap->rows;

		for (i = x, p = 0; i < x_max; i++, p++)
		{
			for (j = y, q = 0; j < y_max; j++, q++)
			{
	//                if ( i < 0 || j < 0 || i > (ftImgSize.x -1) || j > (ftImgSize.y -1) )
	//                    break;

				for (int c = 0; c < 3; c++)
				{
					int ind = (j * ftImgSize.x*4) + i*4 + c;
					if (ind >= (ftImgSize.x * ftImgSize.y * 4) )
						std::cout << "ind: " << ind << std::endl;
					else
						ftImg[ind] = 255;
				}

				//ftImg[(j * ftImgSize.x * 4) + i * 4 + 3] = bitmap->buffer[q	* bitmap->width + p];
			}
		}
	}
}

//--------------------------------------------------

void FreetypeTex::bind(int texUnit)
{
	if(ftTex){
		ftTex->bind(texUnit);
	}
}

//--------------------------------------------------

bool FreetypeTex::ready()
{
	return ftOk;
}

//--------------------------------------------------

int FreetypeTex::getWidth()
{
	return ftImgSize.x;
}

//--------------------------------------------------

int FreetypeTex::getHeight()
{
	return ftImgSize.y;
}

//--------------------------------------------------

TextureManager* FreetypeTex::getTex()
{
	return ftTex;
}

//--------------------------------------------------

FreetypeTex::~FreetypeTex()
{
	if (ftImg) delete ftImg;

	//We don't need the face information now that the display
	//lists have been created, so we free the assosiated resources.
	FT_Done_Face(*ftFace);

	//Ditto for the library.
	FT_Done_FreeType(*ftLib);


	delete ftFace;
	delete ftLib;
	if(ftTex) delete ftTex;


}
}
