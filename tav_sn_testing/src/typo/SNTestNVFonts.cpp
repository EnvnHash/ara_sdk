//
// SNTestNVFonts.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestNVFonts.h"

#define STRINGIFY(A) #A

namespace tav
{
SNTestNVFonts::SNTestNVFonts(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    				SceneNode(_scd, _sceneArgs)
{
	FreeTypeFont** fonts = static_cast<FreeTypeFont**>(scd->ft_fonts);

	if ( _sceneArgs->find("font0") != _sceneArgs->end() )
		font = fonts[ static_cast<int>(_sceneArgs->at("font0")) ];
	else
		std::cout << "SNTestNVFonts font0 definition  not found in setup xml" << std::endl;

	text = new NVText(font, scd->screenWidth, 50.f); // lies font von parameter font0
}

//----------------------------------------------------

void SNTestNVFonts::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	// all parameters should work dynamical
	if ( !inited )
	{
		text->setString("takayt sdj l ");
		text->setDirection(HB_DIRECTION_LTR);	// HB_DIRECTION_LTR: left to right
		// HB_DIRECTION_RTL: right to left
		// HB_DIRECTION_TTB: top to bottom
		text->setAlign(ALIGN_LEFT);
		text->setScript(HB_SCRIPT_LATIN);		// see hb-common.h
		text->setLanguage("es");

		text->setStroke(true);
		text->setStrokeColor(0.f, 0.f, 1.f);
		text->setStrokeWidth(0.1f);

		text->setAlpha(1.f);
		text->setFillColor(1.f, 0.5f, 0.3f);
		text->setFill(true);

		//text->setPointSize(60.f);

		inited = true;
	}

	text->draw(cp);
}

//----------------------------------------------------

void SNTestNVFonts::update(double time, double dt)
{}

//----------------------------------------------------

void SNTestNVFonts::onKey(int key, int scancode, int action, int mods)
{}

//----------------------------------------------------

SNTestNVFonts::~SNTestNVFonts()
{
}
}
