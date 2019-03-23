//
// NVTextBlock.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "NVTextBlock.h"

namespace tav
{
NVTextBlock::NVTextBlock(ShaderCollector* _shCol, FreeTypeFont* _font,
		float _pointSize, int _nrSamples) :
		shCol(_shCol), font(_font), textSize(_pointSize), nrSamples(_nrSamples),
		fboScale(2)
{
	quad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 1.f);

	backShdrBrd = _shCol->getStdColBorder();
	backShdr = _shCol->getStdCol();
	stdTex = _shCol->getStdTexMulti();

	str = "";

	blockSize = glm::vec2(1.f, 1.f);
	blockPos = glm::vec2(0.f, 0.f);
	calcTransMat();

	borderPadding = glm::vec2(10.f, 10.f); // in pixel
	borderWidth = 2.f; // pixel
	borderColor = glm::vec4(0.4f, 0.4f, 0.4f, 1.f);
	backColor = glm::vec4(0.9f, 0.9f, 0.9f, 1.f);

	textColor = glm::vec4(0.f, 0.f, 0.f, 1.f);
	strokeColor = glm::vec4(0.2f, 0.2f, 0.5f, 1.f);
	textStrokeWidth = 20.3f;
	lineHeight = 54.f;

	pixPos = glm::vec2(0.f, 0.f);
}

//----------------------------------------------------

void NVTextBlock::draw(camPar* cp)
{
	if (!init)
	{
		multisampleFbo = new FBO(shCol, int(cp->actFboSize.x*fboScale),
				int(cp->actFboSize.y*fboScale), GL_RGBA8,
				GL_TEXTURE_2D_MULTISAMPLE, true, 1, 1, nrSamples,
				GL_CLAMP_TO_EDGE, false);
		init = true;
	}

	// ------ draw Background -------

	if (drawBorder)
	{
		backShdrBrd->begin();
		backShdrBrd->setUniformMatrix4fv("m_pvm", &transMat[0][0]);
		backShdrBrd->setUniform4fv("col", &backColor[0]);
		backShdrBrd->setUniform4fv("borderColor", &borderColor[0]);
		backShdrBrd->setUniform1f("borderShape", 0.0001f);
		backShdrBrd->setUniform2f("borderWidth",
				borderWidth / (cp->actFboSize.x *fboScale * blockSize.x * 0.5f),
				borderWidth / (cp->actFboSize.y *fboScale * blockSize.y * 0.5f));
	}
	else
	{
		backShdr->begin();
		backShdr->setUniformMatrix4fv("m_pvm", &transMat[0][0]);
	}

	quad->draw();

	// ------ render Typo -------

	if (stringChanged)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		multisampleFbo->bind();
		multisampleFbo->clear();

		for (std::vector<NVText*>::iterator it = lines.begin(); it != lines.end(); ++it)
			(*it)->draw(cp);

		multisampleFbo->unbind();
		stringChanged = true;
	}


	// ------ draw Typo to Screen -------
	scaleFboSize = cp->actFboSize * float(fboScale);

	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);
	stdTex->setUniform1i("nrSamples", nrSamples);
	stdTex->setUniform2fv("fboSize", &scaleFboSize[0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, multisampleFbo->getColorImg());

	quad->draw();
}

//----------------------------------------------------

GLuint NVTextBlock::drawToTex(camPar* cp)
{
	if (!init)
	{
		multisampleFbo = new FBO(shCol, int(cp->actFboSize.x*fboScale),
				int(cp->actFboSize.y*fboScale), GL_RGBA8,
				GL_TEXTURE_2D_MULTISAMPLE, true, 1, 1, nrSamples,
				GL_CLAMP_TO_EDGE, false);
		init = true;
	}

	// ------ draw Background -------

	if (drawBorder)
	{
		backShdrBrd->begin();
		backShdrBrd->setUniformMatrix4fv("m_pvm", &transMat[0][0]);
		backShdrBrd->setUniform4fv("col", &backColor[0]);
		backShdrBrd->setUniform4fv("borderColor", &borderColor[0]);
		backShdrBrd->setUniform1f("borderShape", 0.0001f);
		backShdrBrd->setUniform2f("borderWidth",
				borderWidth / (cp->actFboSize.x *fboScale * blockSize.x * 0.5f),
				borderWidth / (cp->actFboSize.y *fboScale * blockSize.y * 0.5f));
	}
	else
	{
		backShdr->begin();
		backShdr->setUniformMatrix4fv("m_pvm", &transMat[0][0]);
	}

	quad->draw();

	// ------ render Typo -------

	multisampleFbo->bind();
	multisampleFbo->clear();

	for (std::vector<NVText*>::iterator it = lines.begin(); it != lines.end(); ++it)
		(*it)->draw(cp);

	multisampleFbo->unbind();

	return multisampleFbo->getColorImg();
}

//----------------------------------------------------

void NVTextBlock::calcTransMat()
{
	transMat = glm::translate(glm::mat4(1.f),
			glm::vec3(blockPos.x, blockPos.y, 0.f))
			* glm::scale(glm::mat4(1.f),
					glm::vec3(blockSize.x / 2.f, blockSize.y / 2.f, 1.f));

//	pixPos = glm::vec2( (blockPos.x - blockSize.x * 0.5f) * 0.5f + 0.5,
//			1.f - ((blockPos.y + blockSize.y * 0.5f) * 0.5f + 0.5) );
//
//	if (lastCp != 0)
//		pixPos = glm::vec2(pixPos.x * float(lastCp->actFboSize.x) + borderPadding,
//			pixPos.y * float(lastCp->actFboSize.y) + borderPadding);
}

//----------------------------------------------------

void NVTextBlock::setString(std::string _str, camPar* cp)
{
	str = _str;
	if (cp != 0)
	{
		lastCp = cp;

		// separate string
		std::vector<std::string> sepFormat = tav::split(str, "\n");

		lines.clear();

		unsigned int lineCount = 0;
		for (std::vector<std::string>::iterator it = sepFormat.begin();
				it != sepFormat.end(); ++it)
		{
			std::string overflStr = (*it);

			while (std::strlen(overflStr.c_str()) != 0
					&& (lineCount * lineHeight) < (blockSize.y * 0.5f * float(cp->actFboSize.y)))
			{
				lines.push_back( new NVText(font, blockSize.x * 0.5f * float(cp->actFboSize.x)
										- borderPadding.x * 2.f, textSize));

				lines.back()->setPointSize(textSize);
				overflStr = lines.back()->setString(overflStr);

				// set position, convert relative values in pixel values, get left upper corner of textBlock
				glm::vec2 normPos = glm::vec2(
						(blockPos.x - blockSize.x * 0.5f) * 0.5f + 0.5,
						1.f - ((blockPos.y + blockSize.y * 0.5f) * 0.5f + 0.5));

				lines.back()->setPos( normPos.x * float(cp->actFboSize.x) + borderPadding.x,
						normPos.y * float(cp->actFboSize.y) + borderPadding.y
								+ float(lineCount) * lineHeight);

				lines.back()->setDirection(HB_DIRECTION_LTR);// HB_DIRECTION_LTR: left justify
				// HB_DIRECTION_RTL: right justify
				// HB_DIRECTION_TTB: center
				lines.back()->setAlign(xAlign);	// HB_DIRECTION_LTR: left justify

				lines.back()->setScript(HB_SCRIPT_LATIN);	// see hb-common.h
				lines.back()->setLanguage("es");

				lines.back()->setStroke(textStroke);
				lines.back()->setStrokeColor(strokeColor.r, strokeColor.g,
						strokeColor.b);
				lines.back()->setStrokeWidth(textStrokeWidth);

				lines.back()->setAlpha(textColor.a);
				lines.back()->setFillColor(textColor.r, textColor.g,
						textColor.b);
				lines.back()->setFill(true);

				lineCount++;
			}
		}
	}

	stringChanged = true;
}

//---------------------------------------------------------

void NVTextBlock::setFontStyle(bool bold, bool italic)
{
	for (std::vector<NVText*>::iterator it = lines.begin(); it != lines.end(); ++it)
		(*it)->setFontStyle(bold, italic);
}

//----------------------------------------------------

void NVTextBlock::setSize(float _x, float _y)
{
	if (_x != blockSize.x || _y != blockSize.y)
	{
		blockSize = glm::vec2(_x, _y);
		calcTransMat();
		setString(str, lastCp);
	}
}

//----------------------------------------------------

void NVTextBlock::setPos(float _x, float _y)
{
	if (_x != blockPos.x || _y != blockPos.y)
	{
		blockPos = glm::vec2(_x, _y);
		calcTransMat();

		// set position, convert relative values in pixel values, get left upper corner of textBlock
		glm::vec2 normPos = glm::vec2(
				(blockPos.x - blockSize.x * 0.5f) * 0.5f + 0.5,
				1.f - ((blockPos.y + blockSize.y * 0.5f) * 0.5f + 0.5));

		for (std::vector<NVText*>::iterator it = lines.begin(); it != lines.end(); ++it)
			(*it)->setPos(
					normPos.x * float(lastCp->actFboSize.x) + borderPadding.x,
					normPos.y * float(lastCp->actFboSize.y) + borderPadding.y
							+ float(it - lines.begin()) * lineHeight);
	}
}

//----------------------------------------------------

void NVTextBlock::setAlign(alignTypeX _val)
{
	if (_val != xAlign)
	{
		xAlign = _val;
		calcTransMat();
		setString(str, lastCp);
	}
}

//----------------------------------------------------

void NVTextBlock::setBorder(bool _val)
{
	drawBorder = _val;
}

//----------------------------------------------------

void NVTextBlock::setBorderPadding(float _x, float _y)
{
	if (_x != borderPadding.x || _y != borderPadding.y)
	{
		borderPadding.x = _x;
		borderPadding.y = _y;
		calcTransMat();
		setString(str, lastCp);
	}
}

//----------------------------------------------------

void NVTextBlock::setBorderWidth(float _val)
{
	borderWidth = _val;
}

//----------------------------------------------------

void NVTextBlock::setBorderColor(float _r, float _g, float _b, float _a)
{
	borderColor = glm::vec4(_r, _g, _b, _a);
}

//----------------------------------------------------

void NVTextBlock::setBackColor(float _r, float _g, float _b, float _a)
{
	if (backColor.r != _r || backColor.g != _g || backColor.b != _b
			|| backColor.a != _a)
	{
		backColor.r = _r;
		backColor.g = _g;
		backColor.b = _b;
		backColor.a = _a;
		quad->setColor(_r, _g, _b, _a);
	}
}

//----------------------------------------------------

void NVTextBlock::setTextColor(float _r, float _g, float _b, float _a)
{
	if (textColor.r != _r || textColor.g != _g || textColor.b != _b
			|| textColor.a != _a)
	{
		textColor.r = _r;
		textColor.g = _g;
		textColor.b = _b;
		textColor.a = _a;

		for (std::vector<NVText*>::iterator it = lines.begin();
				it != lines.end(); ++it)
		{
			(*it)->setAlpha(textColor.a);
			(*it)->setFillColor(textColor.r, textColor.g, textColor.b);
		}
	}
}

//----------------------------------------------------

void NVTextBlock::setTextStroke(bool _val)
{
	if (_val != textStroke)
	{
		textStroke = _val;
		for (std::vector<NVText*>::iterator it = lines.begin();
				it != lines.end(); ++it)
			(*it)->setStroke(_val);
	}
}

//----------------------------------------------------

void NVTextBlock::setStrokeColor(float _r, float _g, float _b, float _a)
{
	if (strokeColor.r != _r || strokeColor.g != _g || strokeColor.b != _b
			|| strokeColor.a != _a)
	{
		strokeColor.r = _r;
		strokeColor.g = _g;
		strokeColor.b = _b;
		strokeColor.a = _a;

		for (std::vector<NVText*>::iterator it = lines.begin();
				it != lines.end(); ++it)
			(*it)->setStrokeColor(_r, _g, _b);
	}
}

//----------------------------------------------------

void NVTextBlock::setTextStrokeWidth(float _val)
{
	if (_val != textStrokeWidth)
	{
		textStrokeWidth = _val;
		for (std::vector<NVText*>::iterator it = lines.begin();
				it != lines.end(); ++it)
			(*it)->setStrokeWidth(_val);
	}
}

//----------------------------------------------------

void NVTextBlock::setTextSize(float _val)
{
	if (_val != textSize)
	{
		textSize = _val;
		setString(str, lastCp);
	}
}

//----------------------------------------------------

void NVTextBlock::setLineHeight(float _val)
{
	if (_val != lineHeight)
	{
		lineHeight = _val;
		calcTransMat();
		setString(str, lastCp);
	}
}

//----------------------------------------------------

int NVTextBlock::getNrSamples(){

	return nrSamples;
}

//----------------------------------------------------

NVTextBlock::~NVTextBlock()
{
	delete quad;
}
}
