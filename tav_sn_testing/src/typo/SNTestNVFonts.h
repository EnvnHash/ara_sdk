//
// SNTestNVFonts.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#ifndef __SNTestNVFonts__
#define __SNTestNVFonts__

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GLUtils/Typo/FreeTypeFont.h>
#include <GLUtils/Typo/Nvidia/NVText.h>

namespace tav
{

class SNTestNVFonts : public SceneNode
{
public:
	SNTestNVFonts(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	virtual ~SNTestNVFonts();

	void init(TFO* _tfo = nullptr);
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void startThreads(double time, double dt);
	void stopThreads(double time, double dt);
	void cleanUp();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);

private:
	NVText*				text;
	FreeTypeFont*		font;
	bool				inited=false;
};

}

#endif
