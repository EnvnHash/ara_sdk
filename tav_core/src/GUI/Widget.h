//
//  Widget.h
//  tav_core
//
//  Created by Sven Hahne on 29/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <stdio.h>
#include <vector>
#include <string>

#include "GUI/GUIAnimation.h"
#include "GUI/GUIObject.h"
#include "GUI/GUIMat.h"

#include "GeoPrimitives/Quad.h"
#include "Shaders/ShaderCollector.h"

namespace tav
{
class Widget
{
public:
	Widget();
	~Widget();

	void init(std::vector<GUIAnimation*>* _animUpdtQ,
			std::vector<std::vector<GUIAnimation*>::iterator>* _animUpdtQToKill);
	void resize(glm::vec2* _newParentSize);

	void draw(std::vector<glm::vec4*>* _viewports,
			std::vector<glm::mat4*>* _matrixStack, Shaders* guiColShdr,
			Shaders* guiTexShdr);
	void draw(std::vector<glm::vec4*>* _viewports,
			std::vector<glm::mat4*>* _matrixStack, Shaders* objIdShader);
	void add(GUIObject* obj);

	void setPos(int posX, int posY);
	void setPos(int posX, float posY);
	void setPos(float posX, int posY);
	void setPos(float posX, float posY);

	void setSize(int _width, int _height);
	void setSize(int _width, float _height);
	void setSize(float _width, int _height);
	void setSize(float _width, float _height);

	void alignX(alignTypeX _alignX);
	void alignY(alignTypeY _alignY);

	void setPivotX(pivotX _pX);
	void setPivotY(pivotY _pY);

	void setViewport(std::vector<glm::vec4*>* _viewports,
			std::vector<glm::mat4*>* _matrixStack);

	void setBackColor(float r, float g, float b, float a);
	void setBackTex(std::string path);
	void setId(int* baseId);

	int getId();
	GUIObject* getGuiObj(int i);
	GUIObject* getFirstGuiObj();
	GUIObject* getLastGuiObj();
	std::map<int, GUIObject*>* getGuiMap();

	void onCursor(double xpos, double ypos);
	void onMouseButton(int button, int action, int mods);

private:
	bool hasBack;
	bool hasBackTex;

	int widgetBaseId;

	glm::vec2* parentSize;
	glm::vec4 backColor;

	GUIMat widgMat;
	std::vector<GUIMat> widgGuiMat;
	std::vector<glm::vec4*> viewports;

	glm::vec2 relSize;
	glm::vec2 relPos;

	std::vector<GUIObject*> gObjs;
	std::map<int, GUIObject*> guiMap;
	std::vector<GUIAnimation*>* animUpdtQ;
	std::vector<std::vector<GUIAnimation*>::iterator>* animUpdtQToKill;

	Quad* backgr;
	TextureManager* backTex;
};
}
