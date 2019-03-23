//
//  Widget.cpp
//  tav_core
//
//  Created by Sven Hahne on 29/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "Widget.h"

namespace tav
{
Widget::Widget()
{
}

//--------------------------------------------------

void Widget::init(std::vector<GUIAnimation*>* _animUpdtQ,
		std::vector<std::vector<GUIAnimation*>::iterator>* _animUpdtQToKill)
{
	animUpdtQ = _animUpdtQ;
	animUpdtQToKill = _animUpdtQToKill;

	for (std::vector<GUIObject*>::iterator it = gObjs.begin();
			it != gObjs.end(); ++it)
		(*it)->setAnimUpdtQ(_animUpdtQ, _animUpdtQToKill);

	backgr = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f),
			backColor.r, backColor.g, backColor.b, backColor.a);
}

//--------------------------------------------------

void Widget::resize(glm::vec2* _newParentSize)
{
	//widgMat.setParentSize(_newParentSize->x, _newParentSize->y);
}

//--------------------------------------------------

void Widget::draw(std::vector<glm::vec4*>* _viewports,
		std::vector<glm::mat4*>* _matrixStack, Shaders* guiColShdr,
		Shaders* guiTexShdr)
{
	setViewport(_viewports, _matrixStack);

	// draw for display
	if (hasBack && (int) widgGuiMat.size() != 0)
	{
		if (!hasBackTex)
		{
			guiColShdr->begin();
			guiColShdr->setIdentMatrix4fv("m_pvm");
			guiColShdr->setUniform4fv("col", &backColor[0]);
			backgr->draw();

		}
		else
		{
			glm::vec4 white = glm::vec4(1.f);

			guiTexShdr->begin();
			guiTexShdr->setIdentMatrix4fv("m_pvm");
			guiTexShdr->setUniform1i("tex", 0);
			guiTexShdr->setUniform4fv("frontColor", &white[0]);
			glActiveTexture(GL_TEXTURE0);

			backTex->bind();
			backgr->draw();
		}
	}

	for (std::vector<GUIObject*>::iterator it = gObjs.begin();
			it != gObjs.end(); ++it)
	{
		(*it)->draw(_viewports, _matrixStack, guiColShdr, guiTexShdr);

		// erase the last viewport (from the last drawn GUIObject) from the stack
		_matrixStack->pop_back(); // wieso ausserhalb der methode????
		// printf("\n");
	}
}

//--------------------------------------------------

void Widget::draw(std::vector<glm::vec4*>* _viewports,
		std::vector<glm::mat4*>* _matrixStack, Shaders* objIdShader)
{
	setViewport(_viewports, _matrixStack);

	// draw for display
	objIdShader->begin();
	objIdShader->setIdentMatrix4fv("m_pvm");
	objIdShader->setUniform1i("objId", widgetBaseId);
	if (hasBack)
		backgr->draw();

	for (std::vector<GUIObject*>::iterator it = gObjs.begin();
			it != gObjs.end(); ++it)
	{
		(*it)->draw(_viewports, _matrixStack, objIdShader);
		// erase the last viewport (from the last drawn GUIObject) from the stack
		_matrixStack->pop_back();
	}
}

//--------------------------------------------------

void Widget::add(GUIObject* obj)
{
	gObjs.push_back(obj);
}

//--------------------------------------------------

void Widget::setPos(int posX, int posY)
{
	widgMat.setX(posX);
	widgMat.setY(posY);
}

//--------------------------------------------------

void Widget::setPos(int posX, float posY)
{
	widgMat.setX(posX);
	widgMat.setY(posY);
}

//--------------------------------------------------

void Widget::setPos(float posX, int posY)
{
	widgMat.setX(posX);
	widgMat.setY(posY);
}

//--------------------------------------------------

void Widget::setPos(float posX, float posY)
{
	widgMat.setX(posX);
	widgMat.setY(posY);
}

//--------------------------------------------------

void Widget::setSize(int _width, int _height)
{
	widgMat.setWidth(_width);
	widgMat.setHeight(_height);
}

//--------------------------------------------------

void Widget::setSize(int _width, float _height)
{
	widgMat.setWidth(_width);
	widgMat.setHeight(_height);
}

//--------------------------------------------------

void Widget::setSize(float _width, int _height)
{
	widgMat.setWidth(_width);
	widgMat.setHeight(_height);
}

//--------------------------------------------------

void Widget::setSize(float _width, float _height)
{
	widgMat.setWidth(_width);
	widgMat.setHeight(_height);
}

//--------------------------------------------------

void Widget::alignX(alignTypeX _alignX)
{
	widgMat.setAlignX(_alignX);
}

//--------------------------------------------------

void Widget::alignY(alignTypeY _alignY)
{
	widgMat.setAlignY(_alignY);
}

//--------------------------------------------------

void Widget::setPivotX(pivotX _pX)
{
	widgMat.setPivotX(_pX);
}

//--------------------------------------------------

void Widget::setPivotY(pivotY _pY)
{
	widgMat.setPivotY(_pY);
}

//--------------------------------------------------

void Widget::setViewport(std::vector<glm::vec4*>* _viewports,
		std::vector<glm::mat4*>* _matrixStack)
{
	widgMat.setViewport(_viewports->front());
	widgMat.setMatrixStack(_matrixStack);

	// prüfe ob die anzahl der viewports der anzahl widgGuiMat entspricht
	if (static_cast<short>(_viewports->size())
			!= static_cast<short>(widgGuiMat.size()))
		for (short i = 0; i < static_cast<int>(_viewports->size()); i++)
			widgGuiMat.push_back(GUIMat());

	// setze die parentSizes der Transformationsmatrizen des Widget anhand des übergebenen ViewportSets
	std::vector<GUIMat>::iterator matIt = widgGuiMat.begin();
	int i = 0;
	for (std::vector<glm::vec4*>::iterator it = _viewports->begin();
			it != _viewports->end(); ++it, ++matIt, ++i)
	{
		// handover the parameters from the widgMat to the viewport
		(*matIt).setRefMat(&widgMat);
		(*matIt).setViewport(*it);
		(*matIt).setMatrixStack(_matrixStack);

		//std::cout << "Widget get TransViewport: " << glm::to_string(*(*matIt).getTransViewport()) << std::endl;

		// set the viewport with the size and offset of the parent widget
		glViewportIndexedf(i, (*matIt).getTransViewport()->x,
				(*matIt).getTransViewport()->y, (*matIt).getTransViewport()->z,
				(*matIt).getTransViewport()->w);
	}

	// std::cout << "pushing Matrix: " << glm::to_string( *widgMat.getTrans() ) << std::endl;

	// add the new widget Viewports to the viewport stack
	_matrixStack->push_back(widgMat.getTrans());
}

//--------------------------------------------------

void Widget::setBackColor(float r, float g, float b, float a)
{
	backColor = glm::vec4(r, g, b, a);
//        if(hasBack) backgr->setColor(r, g, b, a);
}

//--------------------------------------------------

void Widget::setBackTex(std::string path)
{
	hasBackTex = true;
	backTex = new TextureManager();
	backTex->loadTexture2D(path);
}

//--------------------------------------------------

void Widget::setId(int* baseId)
{
	widgetBaseId = (*baseId);
	(*baseId)++;

	// loop through all objects and set ids
	for (std::vector<GUIObject*>::iterator it = gObjs.begin();
			it != gObjs.end(); ++it)
	{
		guiMap[*baseId] = (*it);
		(*it)->setId(baseId);
	}
}

//--------------------------------------------------

int Widget::getId()
{
	return widgetBaseId;
}

//--------------------------------------------------

GUIObject* Widget::getGuiObj(int i)
{
	GUIObject* out = 0;
	if (static_cast<int>(gObjs.size()) > i)
		out = gObjs[i];
	return out;
}

//--------------------------------------------------

GUIObject* Widget::getFirstGuiObj()
{
	return gObjs.front();
}

//--------------------------------------------------

GUIObject* Widget::getLastGuiObj()
{
	return gObjs.back();
}

//--------------------------------------------------

std::map<int, GUIObject*>* Widget::getGuiMap()
{
	return &guiMap;
}

//--------------------------------------------------

void Widget::onCursor(double xpos, double ypos)
{
}

//---------------------------------------------------------

void Widget::onMouseButton(int button, int action, int mods)
{
}

//--------------------------------------------------

Widget::~Widget()
{
	if (hasBack)
		delete backgr;
	gObjs.clear();
	gObjs.resize(0);
}
}
