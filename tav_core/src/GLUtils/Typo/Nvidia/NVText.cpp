//
// NVText.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Braucht eine Instanz von FreeFontType
//  diese Klasse generiert NVIDIA glPath Objekte
//	bloedes problem: wenn man z.B. die staerke der umrandung eines Schirftzuges aendert,
//  muessen die ganzen Pfade neu generiert werden, ansonsten koennten alle Glyphs einer
//  Schrift von FreeTypeFont verwaltet werden
//  bold und italic scheinen nicht zu gehen... entsprechenden font stattdessen nehmen
//

#include "pch.h"
#include "NVText.h"

#define STRINGIFY(A) #A

namespace tav
{
NVText::NVText(FreeTypeFont* _font, int _maxWidth, float _pointSize,
		NVprAPImode_t apiMode) :
		maxWidth(_maxWidth), font(_font), stroking(true), filling(true), glyphReady(
				false), pointSize(_pointSize), NVprAPImode(apiMode)
{
	if (hasFramebufferSRGB)
	{
		glGetIntegerv(GL_FRAMEBUFFER_SRGB_CAPABLE_EXT, &sRGB_capable);
	}

	if (!glewGetExtension("GL_NV_path_rendering"))
		std::cerr << "GL_NV_path_rendering not supported!!" << std::endl;

	if (!glewGetExtension("GL_EXT_direct_state_access"))
		std::cerr << "GL_NV_path_rendering not supported!!" << std::endl;

	fillColor = glm::vec3(1.f);
	pos = glm::vec2(0.f);
	size = glm::vec2(1.f);

	initShdr();

#ifdef _WIN32
	if (glPathGlyphIndexRangeNV)
	{
		const GLbitfield styleMask = 0;
		const char *font_name = "Arial";
		GLuint base_and_count[2];
		GLenum fontStatus;
		fontStatus = glPathGlyphIndexRangeNV(GL_SYSTEM_FONT_NAME_NV,
				font_name, styleMask,
				path_template, EM_SCALE, base_and_count);
		GLuint arial_base = base_and_count[0];
		GLuint arial_count = base_and_count[1];
		printf("arial_base = %d, count = %d\n", arial_base, arial_count);
		font_name = "Wingdings";
		fontStatus = glPathGlyphIndexRangeNV(GL_SYSTEM_FONT_NAME_NV,
				font_name, styleMask,
				path_template, EM_SCALE, base_and_count);
		GLuint wingding_base = base_and_count[0];
		GLuint wingding_count = base_and_count[1];
		printf("wingding_base = %d, count = %d\n", wingding_base, wingding_count);
		font_name = "Arial Unicode MS";
		fontStatus = glPathGlyphIndexRangeNV(GL_SYSTEM_FONT_NAME_NV,
				font_name, styleMask,
				path_template, EM_SCALE, base_and_count);
		GLuint arialuni_base = base_and_count[0];
		GLuint arialuni_count = base_and_count[1];
		printf("arialuni_base = %d, count = %d\n", arialuni_base, arialuni_count);
	}
#endif

	//-----------------------------------------------------------------

	// Create a null path object to use as a parameter template for creating fonts.
	path_template = glGenPathsNV(1);
	glPathCommandsNV(path_template, 0, NULL, 0, GL_FLOAT, NULL);
	glPathParameteriNV(path_template, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);
	glPathParameterfNV(path_template, GL_PATH_STROKE_WIDTH_NV, 0.1 * emScale); // 10% of emScale

	fontStatus = GL_FONT_TARGET_UNAVAILABLE_NV;

	setPointSize(50);
	sendPathGlyph(); // stroke setting muss vor sendPathGlyp passieren
	sendPathMetrics();

	if (error_count > 0)
	{
		printf("NVpr font setup failed with %d errors\n", error_count);
#ifdef _WIN32
		printf("\nCould it be you don't have freetype6.dll in your current directory?\n");
#endif
		exit(1);
	}
}

//----------------------------------------------------

void NVText::sendPathGlyph()
{
	if (path_template != 0)
	{
		glDeletePathsNV(path_template, 1);
		path_template = glGenPathsNV(1);
		glPathCommandsNV(path_template, 0, NULL, 0, GL_FLOAT, NULL);
		glPathParameteriNV(path_template, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);
		glPathParameterfNV(path_template, GL_PATH_STROKE_WIDTH_NV,
				strokeWidth * emScale);  // 10% of emScale
	}

	switch (NVprAPImode)
	{
	case GLYPH_INDEX_ARRAY:
		if (nvpr_glyph_base == 0)
		{
			nvpr_glyph_base = glGenPathsNV(font->getNumGlyphs());
		}
		else
		{
			glDeletePathsNV(nvpr_glyph_base, font->getNumGlyphs());
			nvpr_glyph_base = glGenPathsNV(font->getNumGlyphs());
		}

		nvpr_glyph_count = font->getNumGlyphs();

		fontStatus = glPathGlyphIndexArrayNV(nvpr_glyph_base,
		GL_FILE_NAME_NV, font->getPath(), fontStyle, // fontStyle
				0, // firstGlyphIndex
				font->getNumGlyphs(), path_template, EM_SCALE);
		break;
	case MEMORY_GLYPH_INDEX_ARRAY:
	{
		if (nvpr_glyph_base == 0)
		{
			nvpr_glyph_base = glGenPathsNV(font->getNumGlyphs());
			nvpr_glyph_count = font->getNumGlyphs();
		}
		else
		{
			glDeletePathsNV(nvpr_glyph_base, font->getNumGlyphs());
			nvpr_glyph_base = glGenPathsNV(font->getNumGlyphs());
		}

		fontStatus = glPathMemoryGlyphIndexArrayNV(nvpr_glyph_base,
		GL_STANDARD_FONT_FORMAT_NV, font->getFileSize(), font->getBinary(),
				fontStyle, // fontStyle
				0, // firstGlyphIndex
				font->getNumGlyphs(), path_template, EM_SCALE);
	}
		break;
	case GLYPH_INDEX_RANGE:
		fontStatus = glPathGlyphIndexRangeNV(GL_FILE_NAME_NV, font->getPath(),
				0, path_template, EM_SCALE, base_and_count);
		nvpr_glyph_base = base_and_count[0];
		nvpr_glyph_count = base_and_count[1];
		break;
	default:
		assert(!"unknown NVprAPImode");
	}
}

//----------------------------------------------------

void NVText::sendPathMetrics()
{
	if (fontStatus == GL_FONT_GLYPHS_AVAILABLE_NV)
	{
		//printf("Font %s is available @ %d for %d glyphs\n", path, nvpr_glyph_base, nvpr_glyph_count);
		GLuint ids[3] =
		{ 0, nvpr_glyph_count - 1, nvpr_glyph_count };
		GLfloat num_glyphs[3] =
		{ -666, -666, -666 };
		glGetPathMetricsNV(GL_FONT_NUM_GLYPH_INDICES_BIT_NV, 3,
		GL_UNSIGNED_INT, &ids, nvpr_glyph_base, 0, num_glyphs);

	}
	else
	{
		error_count++;

		// Driver should always write zeros on failure
		if (NVprAPImode == GLYPH_INDEX_ARRAY)
		{
			assert(base_and_count[0] == 0);
			assert(base_and_count[1] == 0);
		}
		printf("Font glyphs could not be populated (0x%x)\n", fontStatus);

		switch (fontStatus)
		{
		case GL_FONT_TARGET_UNAVAILABLE_NV:
			printf("> Font target unavailable\n");
			break;
		case GL_FONT_UNAVAILABLE_NV:
			printf("> Font unavailable\n");
			break;
		case GL_FONT_CORRUPT_NV:
			printf("> Font corrupt\n");
			break;
		case GL_OUT_OF_MEMORY:
			printf("> Out of memory\n");
			break;
		case GL_INVALID_VALUE:
			printf(
					"> Invalid value for glPathGlyphIndexRangeNV (should not happen)\n");
			break;
		case GL_INVALID_ENUM:
			printf(
					"> Invalid enum for glPathGlyphIndexRangeNV (should not happen)\n");
			break;
		default:
			printf("> UNKNOWN reason (should not happen)\n");
			break;
		}
	}
}

//----------------------------------------------------

void NVText::configureProjection(float width, float height)
{
	float left = 0, right = width, top = 0, bottom = height;

	glMatrixLoadIdentityEXT(GL_PROJECTION);
	glMatrixOrthoEXT(GL_PROJECTION, left, right, bottom, top, -1, 1);
}

//----------------------------------------------------

void NVText::initShdr()
{
	//------ Frag Shader -----------------------------------------

	std::string shdr_Header =
			"#version 430 core\n#pragma optimize(on)\n#extension GL_ARB_separate_shader_objects : enable\n";

	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 fragColor;\n uniform vec3 color; uniform float alpha; void main()\n {\n fragColor = vec4(color, 1.0); fragColor.a = alpha; });
	frag = "// NVText frag shader\n" + shdr_Header + frag;

	const GLchar *source = frag.c_str();

	//   		shdr = shCol->addCheckShaderText("NVText",  vert.c_str(), frag.c_str());

	shdr = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &source);
}

//----------------------------------------------------

std::string NVText::shapeText(std::string text, float width)
{
	std::string retStr = "";
	unsigned int glyph_count;

	scale = pointSize / EM_SCALE;

	float inv_scale = 1.f / scale;
	float x = 0.f;
	float y = -inv_scale * font->getPointSize();

	// Create a buffer for harfbuzz to use
	hb_buffer_t *buf = hb_buffer_create();
	initHbBuf(buf, &text, &glyph_count);

	// check width of string
	int string_width_in_pixels = 0;
	unsigned int overflowPos = 0;
	unsigned int j = 0;
	while (j < glyph_count && overflowPos == 0)
	{
		string_width_in_pixels += glyph_pos[j].x_advance / 64;
		if (string_width_in_pixels > width)
		{
			overflowPos = j;
			string_width_in_pixels -= glyph_pos[j].x_advance / 64;
		}
		++j;
	}

	// if there is an overflow split the string and return the rest
	if (overflowPos != 0)
	{
		// split into words
		std::vector<std::string> words = tav::split(text, ' ');

		text = "";
		unsigned int strLen = 0;
		for (std::vector<std::string>::iterator it = words.begin();
				it != words.end(); ++it)
		{
			strLen += std::strlen((*it).c_str());

			if (strLen < overflowPos)
			{
				if (it != words.begin())
				{
					text += ' ';
					strLen += 1;
				}
				text += (*it);

			}
			else
			{
				retStr += (*it);
				if (it < words.end() - 1)
					retStr += ' ';
			}
		}

		// reinit buffer
		initHbBuf(buf, &text, &glyph_count);

		string_width_in_pixels = 0;
		for (j = 0; j < glyph_count; ++j)
			string_width_in_pixels += glyph_pos[j].x_advance / 64;
	}

	switch (xAlign)
	{
	case ALIGN_LEFT:
		x = 0.f; // left justify
		break;
	case ALIGN_RIGHT:
		x = inv_scale * (width - string_width_in_pixels); // right justify
		break;
	case CENTER_X:
		x = inv_scale * (width / 2.f - string_width_in_pixels / 2.f);  // center
		break;
	default:
		assert(!"unknown x-Align");
	}

	reset();

	for (unsigned int j = 0; j < glyph_count; ++j)
	{
		//GLuint path = nvpr_glyph_base + glyph_info[j].codepoint;
		addGlyph(x + inv_scale * glyph_pos[j].x_offset / 64,
				y + inv_scale * glyph_pos[j].y_offset / 64,
				glyph_info[j].codepoint);

		x += inv_scale * glyph_pos[j].x_advance / 64; // here spacing, sieht aber nicht gut aus...
		y += inv_scale * glyph_pos[j].y_advance / 64;
	}

	y += -inv_scale * (font->getPointSize() * 1.5f);

	/*
	 // kerning
	 if(kerning != NULL) delete kerning;
	 kerning = new GLfloat[glyph_count];
	 kerning[0] = 0.f;

	 glGetPathSpacingNV(GL_ACCUM_ADJACENT_PAIRS_NV,
	 glyph_count, GL_UNSIGNED_BYTE,
	 str.c_str(),
	 path_template,
	 1.f, 1.f, GL_TRANSLATE_X_NV,
	 kerning);
	 */

	hb_buffer_destroy(buf);

	return retStr;
}

//----------------------------------------------------

void NVText::initHbBuf(hb_buffer_t *buf, std::string* text,
		unsigned int* glyph_count)
{
//	if ( glyph_info != 0) delete glyph_info;
//	if ( glyph_pos != 0) delete glyph_pos;

	hb_buffer_clear_contents(buf);
	hb_buffer_set_direction(buf, direction); // or LTR
	hb_buffer_set_script(buf, script); // see hb-unicode.h
	hb_buffer_set_language(buf,
			hb_language_from_string(language, int(strlen(language))));
	//alternatively you can use hb_buffer_set_unicode_funcs(buf, hb_glib_get_unicode_funcs());
	hb_buffer_set_unicode_funcs(buf, hb_ucdn_make_unicode_funcs());

	// Layout the text
	int text_length(int(strlen(text->c_str())));

	hb_buffer_add_utf8(buf, text->c_str(), text_length, 0, text_length);
	hb_shape(font->getHbFont(), buf, NULL, 0);

	// Hand the layout to cairo to render
	glyph_info = hb_buffer_get_glyph_infos(buf, glyph_count);
	glyph_pos = hb_buffer_get_glyph_positions(buf, glyph_count);
}

//----------------------------------------------------

void NVText::draw(camPar* cp)
{
	if (glyphReady)
	{
		GLuint colorLoc = glGetUniformLocation(shdr, "color");
		GLuint alphaLoc = glGetUniformLocation(shdr, "alpha");

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_STENCIL_TEST);

		glClearStencil(0);
		glStencilMask(~0);
		glStencilFunc(GL_NOTEQUAL, 0, ~0U);
		//glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
		glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

		//	if (sRGB_capable) glEnable(GL_FRAMEBUFFER_SRGB_EXT);

		configureProjection(cp->actFboSize.x, cp->actFboSize.y);

		glUseProgram(shdr);
		glUniform1f(alphaLoc, alpha); // black

		glMatrixPushEXT(GL_MODELVIEW);
		glMatrixTranslatefEXT(GL_MODELVIEW, pos.x, pos.y, 0.f);

		if (stroking)
		{
			glUniform3f(colorLoc, strokeColor.r, strokeColor.g, strokeColor.b); // black
			drawStroked(stencil_write_mask);
		}

		if (filling)
		{
			glUniform3f(colorLoc, fillColor.r, fillColor.g, fillColor.b); // yellow
			drawFilled(stencil_write_mask);
		}

		glMatrixPopEXT(GL_MODELVIEW);
		glUseProgram(0);
		glDisable(GL_STENCIL_TEST);

		//	if (sRGB_capable) glDisable(GL_FRAMEBUFFER_SRGB_EXT);
	}
}

//----------------------------------------------------

void NVText::reset()
{
	glyphs.clear();
	xy.clear();
}

//----------------------------------------------------

void NVText::addGlyph(GLfloat x, GLfloat y, GLuint gindex)
{
	glyphs.push_back(gindex);
	xy.push_back(x);
	xy.push_back(y);
}

//----------------------------------------------------

void NVText::drawFilled(GLuint stencil_write_mask)
{
	assert(2 * glyphs.size() == xy.size());

	glMatrixPushEXT(GL_MODELVIEW);
	glMatrixScalefEXT(GL_MODELVIEW, scale, -scale, 1);

	glStencilFillPathInstancedNV(GLsizei(glyphs.size()),
	GL_UNSIGNED_INT, &glyphs[0], nvpr_glyph_base,
	GL_COUNT_UP_NV, stencil_write_mask,
	GL_TRANSLATE_2D_NV, &xy[0]);

	glCoverFillPathInstancedNV(GLsizei(glyphs.size()),
	GL_UNSIGNED_INT, &glyphs[0], nvpr_glyph_base,
	GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
	GL_TRANSLATE_2D_NV, &xy[0]);

	glMatrixPopEXT(GL_MODELVIEW);
}

//----------------------------------------------------

void NVText::drawStroked(GLuint stencil_write_mask)
{
	assert(2 * glyphs.size() == xy.size());

	glMatrixPushEXT(GL_MODELVIEW);
	glMatrixScalefEXT(GL_MODELVIEW, scale, -scale, 1);

	glStencilStrokePathInstancedNV(GLsizei(glyphs.size()),
	GL_UNSIGNED_INT, &glyphs[0], nvpr_glyph_base, 0x1, // reference, stroking specific parameters
			stencil_write_mask,
			GL_TRANSLATE_2D_NV, &xy[0]);

	glCoverStrokePathInstancedNV(GLsizei(glyphs.size()),
	GL_UNSIGNED_INT, &glyphs[0], nvpr_glyph_base,
	GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
	GL_TRANSLATE_2D_NV, &xy[0]);

	glMatrixPopEXT(GL_MODELVIEW);
}

//---------------------------------------------------------

void NVText::setAlpha(float _val)
{
	alpha = _val;
}

//---------------------------------------------------------

void NVText::setAlign(alignTypeX _align)
{
	xAlign = _align;
	reset();
	shapeText(str, maxWidth);
}

//---------------------------------------------------------

void NVText::setFillColor(float _r, float _g, float _b)
{
	fillColor = glm::vec3(_r, _g, _b);
}

//---------------------------------------------------------

void NVText::setDirection(hb_direction_t _dir)
{
	direction = _dir;
	reset();
	shapeText(str, maxWidth);
}

//---------------------------------------------------------

void NVText::setFill(bool _val)
{
	filling = _val;
}

//---------------------------------------------------------

void NVText::setFontStyle(bool _bold, bool _italic)
{
	if (bold != _bold || _italic != italic)
	{
		fontStyle = GL_NONE;
		if (_bold)
			fontStyle |= GL_BOLD_BIT_NV;
		if (_italic)
			fontStyle |= GL_ITALIC_BIT_NV;
		sendPathGlyph();
	}
}

//---------------------------------------------------------

void NVText::setLanguage(const char* _lang)
{
	language = _lang;
	reset();
	shapeText(str, maxWidth);
}

//---------------------------------------------------------

void NVText::setPointSize(float _val)
{
	if (pointSize != _val || !initPointSize)
	{
		initPointSize = true;
		pointSize = _val;
		font->setPointSize(_val);
		reset();
		shapeText(str, maxWidth);
	}
}

//---------------------------------------------------------

void NVText::setPos(float _x, float _y)
{
	pos = glm::vec2(_x, _y);
}

//---------------------------------------------------------

void NVText::setScript(hb_script_t _script)
{
	script = _script;
	reset();
	shapeText(str, maxWidth);
}

//---------------------------------------------------------

void NVText::setSize(float _x, float _y)
{
	size = glm::vec2(_x, _y);
}

//---------------------------------------------------------

std::string NVText::setString(std::string _str)
{
	str = _str;
	reset();
	std::string retStr = shapeText(_str, maxWidth);
	glyphReady = true;

	return retStr;
}

//---------------------------------------------------------

void NVText::setStroke(bool _val)
{
	stroking = _val;
}

//---------------------------------------------------------

void NVText::setStrokeWidth(float _width)
{
	if (_width != strokeWidth)
	{
		strokeWidth = _width;
		sendPathGlyph();
		//font->setStrokeWidth(_width);
		//reset();
		//shapeText( str, maxWidth);
	}
}

//---------------------------------------------------------

void NVText::setStrokeColor(float _r, float _g, float _b)
{
	strokeColor = glm::vec3(_r, _g, _b);
}

//----------------------------------------------------

NVText::~NVText()
{
	glDeleteProgram(shdr);
	glDeletePathsNV(nvpr_glyph_base, nvpr_glyph_count);

}
}
