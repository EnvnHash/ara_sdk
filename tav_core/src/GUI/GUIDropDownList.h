//
//  GUIDropDownList.h
//  tav_core
//
//  Created by Sven Hahne on 29/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <stdio.h>
#include "GUIObject.h"
#include "../GLUtils/Typo/FreetypeTex.h"
#include "GeoPrimitives/Quad.h"

namespace tav
{
class GUIDropDownList: public GUIObject
{
public:
	GUIDropDownList();
	GUIDropDownList(glm::vec2 _pos, glm::vec2 _size);
	~GUIDropDownList();

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
	//        void setAction(widgetEvent _event, GUICallbackFunction _action);

private:
	FreetypeTex* label;
	bool firstRun;
	glm::vec4 animBackCol;
};
}
