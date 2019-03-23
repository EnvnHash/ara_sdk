//
//  GUIObject.h
//  tav_core
//
//  Created by Sven Hahne on 29/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <stdio.h>
#include <map>
#include <functional>
#include <glm/glm.hpp>

#include "GUIAnimation.h"
#include "GUI/GUIMat.h"
#include "../GeoPrimitives/GeoPrimitive.h"
#include "GeoPrimitives/Quad.h"
#include "../GLUtils/Typo/FreetypeTex.h"

namespace tav
{
enum widgetEvent
{
	NONE = 0,
	LEFT_CLICK = 1,
	RIGHT_CLICK = 2,
	SWIPE_LEFT = 3,
	SWIPE_RIGHT = 4,
	SWIPE_UP = 5,
	SWIPE_DOWN = 6,
	ZOOM_IN = 7,
	ZOOM_OUT = 8,
	ROTATE_LEFT = 9,
	ROTATE_RIGHT = 10,
	LEFT_PRESS = 11,
	RIGHT_PRESS = 12,
	LEFT_RELEASE = 13,
	RIGHT_RELEASE = 14
};

class GUIObject
{
public:
	GUIObject();
	GUIObject(glm::vec2 _pos, glm::vec2 _size);
	virtual ~GUIObject();

	virtual void init();
	virtual void draw(std::vector<glm::vec4*>* _viewports,
			std::vector<glm::mat4*>* _matrixStack, Shaders* _objIdShader);
	virtual void draw(std::vector<glm::vec4*>* _viewports,
			std::vector<glm::mat4*>* _matrixStack, Shaders* _guiColShdr,
			Shaders* _guiTexShdr)=0;
	virtual void drawLabel(Shaders* _guiTexShdr);
	virtual void startAnim(widgetEvent _event)=0;
	virtual int getId();

	virtual void setPos(int posX, int posY);
	virtual void setPos(int posX, float posY);
	virtual void setPos(float posX, int posY);
	virtual void setPos(float posX, float posY);
	virtual void setSize(int _width, int _height);
	virtual void setSize(int _width, float _height);
	virtual void setSize(float _width, int _height);
	virtual void setSize(float _width, float _height);
	virtual void setAlignX(alignTypeX _alignX);
	virtual void setAlignY(alignTypeY _alignY);
	virtual void setPivotX(pivotX _pX);
	virtual void setPivotY(pivotY _pY);
	virtual void setViewport(std::vector<glm::vec4*>* _viewports,
			std::vector<glm::mat4*>* _matrixStack);

	virtual void setAnimUpdtQ(std::vector<GUIAnimation*>* _animUpdtQ,
			std::vector<std::vector<GUIAnimation*>::iterator>* _animUpdtQToKill);
	virtual void setColor(float r, float g, float b, float a);
	virtual void setHighLightColor(widgetEvent _event, float r, float g,
			float b, float a);
	virtual void setBackColor(float r, float g, float b, float a);
	virtual void setBackTex(std::string path);
	virtual void setTextures(std::vector<std::string> paths)=0;
	virtual void setId(int* _id);
	virtual void setFont(const char* path, int size = 16);
	virtual void setLabel(std::string _label, alignTypeX alignH = CENTER_X,
			alignTypeY alignV = CENTER_Y);
	virtual void setLabelMat();
	virtual void setLabelBorder(float left, float right, float top,
			float bottom);
	virtual void setLabelBorder(int left, int right, int top, int bottom);
	virtual void setParentSize(float _width, float _height);

	// actions
	virtual void setAction(widgetEvent _event, GUICallbackFunction _action);
	virtual void setActionAnim(widgetEvent _event, guiAnimFunc _animFunc);
	virtual void setActionAnimDur(guiAnimFunc _animFunc, double _dur);
	virtual void callAction(widgetEvent _event);
	virtual void removeAnimFromQ();

	bool hasLabel;
	bool hasBackTex;
	int objId;

	GUIMat objMat;
	std::vector<GUIMat> objGuiMat;
	std::vector<glm::mat4> objTransMats;
	std::vector<glm::mat4> labelMats;
	std::vector<glm::vec4*> viewports;

	glm::mat4* widgMatr = 0;

	glm::vec4 color;
	glm::vec4 backColor;
	glm::vec4 transp;
	glm::vec4 border;
	glm::ivec4 iBorder;

	alignTypeX labelAlignX;
	alignTypeY labelAlignY;
	unitType borderType;

	GUIAnimation anim;

	GeoPrimitive* back;
	GeoPrimitive* label;

	FreetypeTex* ftTex;
	TextureManager* backTex;

	std::map<widgetEvent, GUICallbackFunction> actions;
	std::map<widgetEvent, guiAnimFunc> actionAnimFuncs;
	std::map<widgetEvent, glm::vec4> highLightCol;

	std::vector<GUIAnimation*>* animUpdtQ;
	std::vector<std::vector<GUIAnimation*>::iterator>* animUpdtQToKill;
};
}
