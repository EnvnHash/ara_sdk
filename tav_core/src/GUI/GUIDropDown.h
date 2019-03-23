//
//  GUIDropDown.h
//  tav_core
//
//  Created by Sven Hahne on 29/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//



#pragma once

#include <stdio.h>
#include "GUIObject.h"
#include "GUIDropDownList.h"
#include "../GLUtils/Typo/FreetypeTex.h"
#include "GeoPrimitives/Quad.h"

namespace tav
{
class GUIDropDown: public GUIObject
{
public:
	GUIDropDown();
	GUIDropDown(glm::vec2 _pos, glm::vec2 _size);
	~GUIDropDown();

	void init();
	void draw(std::vector<glm::vec4*>* _viewports,
			std::vector<glm::mat4*>* _matrixStack, Shaders* _guiColShdr,
			Shaders* _guiTexShdr);
	void startAnim(widgetEvent _event);
	void setTextures(std::vector<std::string> paths)
	{
	}
	;
	void setDropColor(float r, float g, float b, float a);
	void setDropPos(int _x, int _y);
	void setDropPos(int _x, float _y);
	void setDropPos(float _x, int _y);
	void setDropPos(float _x, float _y);
	void setDropSize(int _width, int _height);
	void setDropSize(int _width, float _height);
	void setDropSize(float _width, int _height);
	void setDropSize(float _width, float _height);
	void setDropAlignX(alignTypeX _alignX);
	void setDropAlignY(alignTypeY _alignY);
	void setDropPivotX(pivotX _pivotX);
	void setDropPivotY(pivotY _pivotY);
	void resetBackCol();
	void addOption(std::string name, GUICallbackFunction _action);
//        void setAction(widgetEvent _event, GUICallbackFunction _action);

private:
	FreetypeTex* label;

	bool firstRun;
	bool isSelected;

	GUIDropDownList* dropDownList;

	glm::vec4 animBackCol;

	std::vector<std::pair<std::string, GUICallbackFunction> > options;
};
}
