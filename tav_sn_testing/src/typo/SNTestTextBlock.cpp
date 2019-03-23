//
// SNTestTextBlock.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestTextBlock.h"

#define STRINGIFY(A) #A

namespace tav
{
SNTestTextBlock::SNTestTextBlock(sceneData* _scd, std::map<std::string, float>* _sceneArgs) : SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	FreeTypeFont** fonts = static_cast<FreeTypeFont**>(scd->ft_fonts);

	if ( _sceneArgs->find("font0") != _sceneArgs->end() )
		textBlock = new NVTextBlock(shCol, fonts[ static_cast<int>(_sceneArgs->at("font0")) ] );
	else
		std::cout << "SNTestNVFonts font0 definition  not found in setup xml" << std::endl;
}

//----------------------------------------------------

void SNTestTextBlock::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	if (!isInited)
	{
		textBlock->setAlign(CENTER_X);
		textBlock->setString("tasch asyc uasg\nBlu bladbjb db dhfkjsb kbds kfhbsi hdbf", cp);
		textBlock->setBackColor(0.5f, 0.5f, 0.5f, 0.3f);
		textBlock->setStrokeColor(0.4f, 0.f, 0.f, 1.f);
		textBlock->setTextStroke(true);
		textBlock->setTextStrokeWidth(0.1f);
		textBlock->setTextColor(1.f, 1.f, 0.5f, 1.f);
		isInited = true;
	}

	textBlock->draw(cp);
}

//----------------------------------------------------

void SNTestTextBlock::update(double time, double dt)
{}

//----------------------------------------------------

void SNTestTextBlock::onKey(int key, int scancode, int action, int mods)
{}

//----------------------------------------------------

SNTestTextBlock::~SNTestTextBlock()
{
}
}
