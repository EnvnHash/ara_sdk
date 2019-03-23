//
//  GUIButton.h
//  tav_core
//
//  Created by Sven Hahne on 29/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//


#pragma once

#include <stdio.h>
#include "GUIObject.h"
#include "GeoPrimitives/Quad.h"

namespace tav
{
class GUIButton: public GUIObject
{
public:
	GUIButton();
	~GUIButton();

	void init();
	void draw(std::vector<glm::vec4*>* _viewports,
			std::vector<glm::mat4*>* _matrixStack, Shaders* _guiColShdr,
			Shaders* _guiTexShdr);
	void startAnim(widgetEvent _event);
	void setTextures(std::vector<std::string> paths)
	{
	}
	;
	void resetBackCol();

private:
	bool firstRun;
	glm::vec4 animBackCol;
};
}
