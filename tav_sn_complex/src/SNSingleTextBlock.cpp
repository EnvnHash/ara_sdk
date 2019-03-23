//
// SNSingleTextBlock.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNSingleTextBlock.h"

#define STRINGIFY(A) #A

namespace tav
{
SNSingleTextBlock::SNSingleTextBlock(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    				SceneNode(_scd, _sceneArgs), text("")
{
	// load colors
	cols = new glm::vec4[2];

	for (int i=0;i<2;i++)
	{
		std::string key = "col"+std::to_string(i);

		if ( _sceneArgs->find(key) != _sceneArgs->end() )
		{
			std::cout << "assign _Cols[" << i << "]" << " scd->colors[" <<  static_cast<unsigned int>( sceneArgs->at(key) ) << "]" << std::endl;

			for (int j=0;j<4;j++)
				cols[i][j] = scd->colors[ static_cast<unsigned int>( sceneArgs->at(key) ) ][j];

			std::cout << "cols[" << i << "]" << glm::to_string( cols[i] ) << std::endl;

		} else
		{
			std::cerr << "SceneNode::getChanCols Error: no" << key << " definition in Setup XML.file!!!" << std::endl;
		}
	}


	FreeTypeFont** fonts = static_cast<FreeTypeFont**>(scd->ft_fonts);
	if ( _sceneArgs->find("font0") != _sceneArgs->end() )
		textBlock = new NVTextBlock(shCol, fonts[ static_cast<int>(_sceneArgs->at("font0")) ], 20.f, 4 );
	else
		std::cout << "SNSingleTextBlock font0 definition not found in setup xml" << std::endl;

	////	textBlock->setFontStyle(true);
	textBlock->setBorder(false);
	textBlock->setTextStroke(false);
	textBlock->setBorderPadding(40.f, 40.f);
	//	textBlock->setSize(0.4f, 0.2f);

	addOscStringPar("text", "");

	addPar("textSize", &textSize);
	addPar("alpha", &alpha);
	addPar("paddingX", &paddingX );
	addPar("paddingY", &paddingY );
	addPar("lineHeight", &lineHeight );
}

//----------------------------------------------------

void SNSingleTextBlock::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	/*
    	if (!isInited)
    	{
    		textBlock->setString("bkjbkjbk jb jkb jkb jkb kjb kjb", cp);
    		isInited = true;
    	}
	 */

	if ( std::strcmp(getOscStringPar("text").c_str(), text.c_str()) != 0 )
	{
		text = getOscStringPar("text");
		textBlock->setString(text, cp);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	textBlock->setTextColor(cols[1].r, cols[1].g, cols[1].b, cols[1].a * alpha);
	textBlock->setBackColor(cols[0].r, cols[0].g, cols[0].b, cols[0].a * alpha);

	textBlock->setTextSize( textSize );
	textBlock->setSize(_medScale->get().x, _medScale->get().y);
	textBlock->setPos(_medTrans->get().x, _medTrans->get().y);
	textBlock->setBorderPadding( paddingX, paddingY );
	textBlock->setLineHeight( lineHeight );
	textBlock->draw(cp);
}

//----------------------------------------------------

void SNSingleTextBlock::update(double time, double dt)
{}

//----------------------------------------------------

void SNSingleTextBlock::onKey(int key, int scancode, int action, int mods)
{}

//----------------------------------------------------

SNSingleTextBlock::~SNSingleTextBlock()
{
}
}
