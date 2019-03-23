//
//  GUIDropDown.cpp
//  tav_core
//
//  Created by Sven Hahne on 29/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "GUIDropDown.h"

namespace tav
{
GUIDropDown::GUIDropDown() :
		GUIObject()
{
	init();
}

//--------------------------------------------------

GUIDropDown::GUIDropDown(glm::vec2 _pos, glm::vec2 _size) :
		GUIObject(_pos, _size)
{
	init();
}

//--------------------------------------------------

void GUIDropDown::init()
{
	firstRun = true;
	isSelected = false;

	back = new Quad(-1.f, -1.f, 2.f, 2.f);
	dropDownList = new GUIDropDownList();

	setAction(LEFT_CLICK, [this]()
	{
		isSelected = !isSelected;
		//printf("isSelected %d\n", isSelected);
		});
}

//--------------------------------------------------

void GUIDropDown::draw(std::vector<glm::vec4*>* _viewports,
		std::vector<glm::mat4*>* _matrixStack, Shaders* _guiColShdr,
		Shaders* _guiTexShdr)
{
	if (firstRun)
	{
		animBackCol = backColor;
		firstRun = false;
	}

	printf("\n GUIDropDown::draw \n");
	setViewport(_viewports, _matrixStack);

	if (!hasBackTex)
	{
		_guiColShdr->begin();
		_guiColShdr->setIdentMatrix4fv("m_pvm");
		_guiColShdr->setUniform4fv("col", &animBackCol[0]);

	}
	else
	{
		glm::vec4 white = glm::vec4(1.f);
		_guiTexShdr->begin();
		_guiTexShdr->setIdentMatrix4fv("m_pvm");
		_guiTexShdr->setUniform1i("tex", 0);
		_guiTexShdr->setUniform4fv("frontColor", &white[0]);
		glActiveTexture(GL_TEXTURE0);
		backTex->bind();
	}

	back->draw();

	if (hasLabel)
		drawLabel(_guiTexShdr);

	if (isSelected)
	{
		/*
		 for( std::vector< std::vector<glm::vec4*>* >::iterator it= _viewports->begin(); it != _viewports->end(); ++it)
		 for( std::vector<glm::vec4*>::iterator sit= (*it)->begin(); sit != (*it)->end(); ++sit)
		 std::cout << glm::to_string( *(*sit) ) << std::endl;
		 
		 // drop down menu has can be anywhere on the screen, therefore take the first viewport
		 // set the viewport with the size and offset of the parent widget
		 for (short i=0;i<static_cast<short>(_viewports->front()->size());i++)
		 glViewportIndexedf(i, _viewports->front()->at(i)->x, _viewports->front()->at(i)->y,
		 _viewports->front()->at(i)->z, _viewports->front()->at(i)->w);
		 
		 std::vector<GUIMat>::iterator matIt = viewportMats.begin();
		 std::vector<glm::mat4>::iterator otIt = objTransMats.begin();
		 for( std::vector<glm::vec4*>::iterator it= _viewports->front()->begin(); it != _viewports->front()->end(); ++it, ++matIt, ++otIt)
		 {
		 // handover the parameters from the widgMat to the viewportMats
		 (*matIt).setRefMat(&objMat);
		 (*matIt).setParentSize((*it)->b, (*it)->a);
		 (*otIt) = glm::mat4(1.f);
		 //                (*otIt) = *(*matIt).getTrans();
		 }
		 
		 dropDownList->draw(_viewports, _guiColShdr, _guiTexShdr);
		 */
	}
}

//--------------------------------------------------

void GUIDropDown::startAnim(widgetEvent _event)
{
	animBackCol = backColor;

	if (!anim.isRunning())
	{
		anim.setAnimVal<glm::vec4>(actionAnimFuncs[_event], &animBackCol,
				&highLightCol[_event], [this]()
				{	return this->resetBackCol();});
		anim.start();
		// push the animation to the animation queue in Root Widget for optimized updating
		animUpdtQ->push_back(&anim);
	}
}

//--------------------------------------------------

void GUIDropDown::addOption(std::string name, GUICallbackFunction _action)
{
	options.push_back(std::make_pair(name, _action));
}

//--------------------------------------------------

//    void GUIDropDown::setAction(widgetEvent _event, GUICallbackFunction _action)
//    {
//        actions[_event] = _action;
//    }

//--------------------------------------------------

void GUIDropDown::resetBackCol()
{
	animBackCol = backColor;
	removeAnimFromQ();
}

//--------------------------------------------------

void GUIDropDown::setDropColor(float r, float g, float b, float a)
{
	dropDownList->setBackColor(r, g, b, a);
}

//--------------------------------------------------

void GUIDropDown::setDropPos(int _x, int _y)
{
	dropDownList->setPos(_x, _y);
}

//--------------------------------------------------

void GUIDropDown::setDropPos(int _x, float _y)
{
	dropDownList->setPos(_x, _y);
}

//--------------------------------------------------

void GUIDropDown::setDropPos(float _x, int _y)
{
	dropDownList->setPos(_x, _y);
}

//--------------------------------------------------

void GUIDropDown::setDropPos(float _x, float _y)
{
	dropDownList->setPos(_x, _y);
}

//--------------------------------------------------

void GUIDropDown::setDropAlignX(alignTypeX _alignX)
{
	dropDownList->setAlignX(_alignX);
}

//--------------------------------------------------

void GUIDropDown::setDropAlignY(alignTypeY _alignY)
{
	dropDownList->setAlignY(_alignY);
}

//--------------------------------------------------

void GUIDropDown::setDropPivotX(pivotX _pivotX)
{
	dropDownList->setPivotX(_pivotX);
}

//--------------------------------------------------

void GUIDropDown::setDropPivotY(pivotY _pivotY)
{
	dropDownList->setPivotY(_pivotY);
}

//--------------------------------------------------

GUIDropDown::~GUIDropDown()
{
}
}
