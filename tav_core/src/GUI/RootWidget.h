//
//  RootWidget.h
//  tav_core
//
//  Created by Sven Hahne on 29/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <stdio.h>
#include <vector>

#include "GLUtils/VAO.h"
#include "GLUtils/FBO.h"
#include "GLUtils/TFO.h"
#include "Widget.h"

#include <GLFW/glfw3.h>

namespace tav
{
class RootWidget
{
public:
	RootWidget(ShaderCollector* _shCol, int _scrWidth, int _scrHeight,
			int _nrViewports = 1);
	~RootWidget();

	void add(Widget* widg);
	void init();
	void draw();
	void update(double time, double dt);

	void getObjFromMap(uint8_t nrPoints);
	void onCursor(double xpos, double ypos);
	void onMouseButton(int button, int action, int mods, double xPos,
			double yPos);
	void onKey(int key, int scancode, int action, int mods);

	void setCmd(double xpos, double ypos, widgetEvent _event);
	void addColShader(int _nrViewports);
	void addTexShader(int _nrViewports);
	void addObjIdShader(int _nrViewports);
	void addTouchRecShader();

private:
	std::vector<Widget*> widgets;
	std::map<int, Widget*> widgMap;

	std::vector<std::pair<double, widgetEvent> > actionQ;
	std::vector<widgetEvent> reqActionQ;
	std::vector<GUIAnimation*> animUpdtQ;
	std::vector<std::vector<GUIAnimation*>::iterator> animUpdtQToKill;

	std::vector<glm::vec4*> viewports;
	std::vector<glm::mat4*> matrixStack;

	ShaderCollector* shCol;

	Shaders* objIdShader;
	Shaders* guiTexShader;
	Shaders* guiColShader;
	Shaders* touchRecShader;

	FBO* objIdFbo;
	TFO* ptrFdbk;
	VAO* ptrVao;

	glm::vec2* actCursorPos;
	glm::vec4 rootViewport;
	glm::mat4 rootMat;

	widgetEvent actEvent;

	bool guiHasChanged;
	bool inited;

	int maxNrMultTouch;
	int scrWidth;
	int scrHeight;
	int nrViewports;

	glm::vec2 screenSize;
};
}
