//
// NVText.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//
#ifndef __NVText__
#define __NVText__

#pragma once

#include <iostream>

#include "string_utils.h"
#include "headers/tav_types.h"
#include "GLUtils/Typo/FreeTypeFont.h"
#include "GUI/GUIMat.h"

// Possible return tokens from glPathGlyphIndexRangeNV:
#define GL_FONT_GLYPHS_AVAILABLE_NV                         0x9368
#define GL_FONT_TARGET_UNAVAILABLE_NV                       0x9369
#define GL_FONT_UNAVAILABLE_NV                              0x936A
#define GL_FONT_CORRUPT_NV                                  0x936B
#define GL_STANDARD_FONT_FORMAT_NV                          0x936C
#define GL_FONT_NUM_GLYPH_INDICES_BIT_NV                    0x20000000

namespace tav
{

class NVText
{
public:
	enum NVprAPImode_t
	{
		GLYPH_INDEX_ARRAY, MEMORY_GLYPH_INDEX_ARRAY, GLYPH_INDEX_RANGE,
	} NVprAPImode;

	NVText(FreeTypeFont* _font, int _maxWidth, float _pointSize,
			NVprAPImode_t apiMode = MEMORY_GLYPH_INDEX_ARRAY);
	virtual ~NVText();

	void configureProjection(float width, float height);
	void initShdr();
	void sendPathGlyph();
	void sendPathMetrics();
	std::string shapeText(std::string text, float width);
	void initHbBuf(hb_buffer_t *buf, std::string* text,
			unsigned int* glyph_count);

	void draw(camPar* cp);

	void reset();
	void addGlyph(GLfloat x, GLfloat y, GLuint gindex);
	void drawFilled(GLuint stencil_write_mask);
	void drawStroked(GLuint stencil_write_mask);

	void setAlpha(float _val);
	void setAlign(alignTypeX _align);
	void setDirection(hb_direction_t _dir);
	void setFill(bool _val);
	void setFillColor(float _r, float _g, float _b);
	void setFontStyle(bool bold, bool italic);
	void setLanguage(const char* _lang);
	void setPointSize(float _val);
	void setPos(float _x, float _y);
	void setScript(hb_script_t _script);
	void setSize(float _x, float _y);
	std::string setString(std::string _str);
	void setStrokeColor(float _r, float _g, float _b);
	void setStrokeWidth(float _width);
	void setStroke(bool _val);

private:

	float scale;
	std::vector<GLuint> glyphs;
	std::vector<GLfloat> xy;

	FreeTypeFont* font;

	hb_glyph_info_t* glyph_info = 0;
	hb_glyph_position_t* glyph_pos = 0;
	hb_direction_t direction = HB_DIRECTION_LTR;
	hb_script_t script = HB_SCRIPT_LATIN; // se hb-common.h
	const char* language = "es"; // se hb-common.h
	alignTypeX xAlign = ALIGN_LEFT;

	glm::vec2 pos;
	glm::vec2 size;

	glm::mat3 view;
	glm::vec3 fillColor;
	glm::vec3 strokeColor;

	GLuint shdr;
	GLint sRGB_capable;

	std::string str;

	bool hasFramebufferSRGB;
	bool stroking;
	bool filling;
	bool glyphReady;
	bool bold = false;
	bool italic = false;
	bool initPointSize = false;

	int background;
	int maxWidth;
	int error_count = 0;

	float pointSize = 1.f;
	float alpha = 1.f;
	float textX = 0.f;
	float strokeWidth = 0.1f;
	GLfloat* kerning = NULL;

	const GLuint stencil_write_mask = ~0U;

	GLenum fontStatus;
	GLuint path_template = 0;

	GLuint nvpr_glyph_base = 0;
	GLuint nvpr_glyph_count;
	GLuint base_and_count[2];
	GLbitfield fontStyle = GL_NONE;

	static const int emScale = 2048;
	const float EM_SCALE = 2048;
};
}

#endif
