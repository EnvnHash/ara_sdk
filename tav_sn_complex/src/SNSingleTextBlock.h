//
// SNSingleTextBlock.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#ifndef __SNSingleTextBlock__
#define __SNSingleTextBlock__

#pragma once

#include <SceneNode.h>
#include "GLUtils/Typo/FreeTypeFont.h"
#include <GLUtils/Typo/Nvidia/NVTextBlock.h>
#include <GLUtils/Typo/Nvidia/NVText.h>

namespace tav
{

class SNSingleTextBlock : public SceneNode
{
public:
	SNSingleTextBlock(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	virtual ~SNSingleTextBlock();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);

private:
    ShaderCollector*				shCol;
    glm::vec4*				cols;
	FreeTypeFont*			font;
	NVTextBlock*			textBlock;
	std::string				text;
	bool					isInited=false;
	float					textSize=20.f;
	float					alpha=1.f;
	float					paddingX=20.f;
	float					paddingY=20.f;
	float					lineHeight=20.f;
 };

}

#endif
