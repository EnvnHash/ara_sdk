//
// NVTextBlock.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#ifndef __NVTextBlock__
#define __NVTextBlock__

#pragma once

#include "GeoPrimitives/Quad.h"
#include "NVText.h"
#include "GLUtils/FBO.h"

namespace tav
{

class NVTextBlock
{
public:
	NVTextBlock(ShaderCollector* _shCol, FreeTypeFont* _font, float _pointSize =
			50.f, int _nrSamples = 4);
	virtual ~NVTextBlock();

	void draw(camPar* cp);
	GLuint drawToTex(camPar* cp);
	void calcTransMat();
	void setString(std::string _str, camPar* cp);
	void setFontStyle(bool bold, bool italic);

	void setSize(float _x, float _y);
	void setPos(float _x, float _y);

	void setAlign(alignTypeX _val);
	void setBorder(bool _val);
	void setBorderPadding(float _x, float _y); // in pixel
	void setBorderWidth(float _val); // pixel
	void setBorderColor(float _r, float _g, float _b, float _a);
	void setBackColor(float _r, float _g, float _b, float _a);
	void setTextColor(float _r, float _g, float _b, float _a);
	void setTextStroke(bool _val);
	void setStrokeColor(float _r, float _g, float _b, float _a);
	void setTextStrokeWidth(float _val);
	void setTextSize(float _val); // in points
	void setLineHeight(float _val);

	int getNrSamples();
	inline glm::vec2* getSize() { return &blockSize; }
	inline float getFontSize() { return textSize;  }

private:
	FreeTypeFont* font;
	ShaderCollector* shCol;
	Shaders* backShdrBrd;
	Shaders* backShdr;
	Shaders* stdTex;
	Quad* quad;
	FBO* multisampleFbo;

	std::vector<NVText*> lines;
	std::string str;

	glm::vec2 pixPos;
	glm::vec2 blockSize;
	glm::vec2 blockPos;
	glm::vec2 scaleFboSize;
	glm::vec4 borderColor;
	glm::vec4 backColor;
	glm::vec4 textColor;
	glm::vec4 strokeColor;

	glm::mat4 transMat;

	bool wrap = true;
	bool init = false;
	bool textStroke = false;
	bool drawBorder = false;
	bool stringChanged = false;

	glm::vec2 borderPadding;
	float textSize = 50.f;
	float textStrokeWidth = 0.1f;
	float lineHeight = 52.f;

	int borderWidth;
	int nrSamples;
	int fboScale;

	camPar* lastCp = 0;
	alignTypeX xAlign = ALIGN_LEFT;

};

}

#endif
