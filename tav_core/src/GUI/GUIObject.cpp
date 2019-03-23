//
//  GUIObject.cpp
//  tav_core
//
//  Created by Sven Hahne on 29/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "GUIObject.h"

namespace tav
{
GUIObject::GUIObject()
{
	init();
}

//--------------------------------------------------

GUIObject::GUIObject(glm::vec2 _pos, glm::vec2 _size)
{
	setPos(_pos.x, _pos.y);
	setSize(_size.x, _size.y);
	init();
}

//--------------------------------------------------

void GUIObject::init()
{
	objId = 0;
	hasLabel = false;
	hasBackTex = false;
	border = glm::vec4(0.f);
	transp = glm::vec4(0.f);
}

//--------------------------------------------------

void GUIObject::draw(std::vector<glm::vec4*>* _viewports,
		std::vector<glm::mat4*>* _matrixStack, Shaders* _objIdShader)
{
	setViewport(_viewports, _matrixStack);

	_objIdShader->setIdentMatrix4fv("m_pvm");
	_objIdShader->setUniform1i("objId", objId);

	back->draw();
}

//--------------------------------------------------

void GUIObject::drawLabel(Shaders* _guiTexShdr)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);

	_guiTexShdr->begin();
	_guiTexShdr->setUniformMatrix4fv("m_pvm", objMat.getTransPtr());
	_guiTexShdr->setUniform1i("tex", 0);
	_guiTexShdr->setUniform4fv("backColor", &transp[0]);
	_guiTexShdr->setUniform4fv("frontColor", &color[0]);

	ftTex->bind();
	label->draw();
}

//--------------------------------------------------

int GUIObject::getId()
{
	return objId;
}

//--------------------------------------------------

void GUIObject::setPos(int posX, int posY)
{
	objMat.setX(posX);
	objMat.setY(posY);
}

//--------------------------------------------------

void GUIObject::setPos(int posX, float posY)
{
	objMat.setX(posX);
	objMat.setY(posY);
}

//--------------------------------------------------

void GUIObject::setPos(float posX, int posY)
{
	objMat.setX(posX);
	objMat.setY(posY);
}

//--------------------------------------------------

void GUIObject::setPos(float posX, float posY)
{
	objMat.setX(posX);
	objMat.setY(posY);
}

//--------------------------------------------------

void GUIObject::setSize(int _width, int _height)
{
	objMat.setWidth(_width);
	objMat.setHeight(_height);
}

//--------------------------------------------------

void GUIObject::setSize(int _width, float _height)
{
	objMat.setWidth(_width);
	objMat.setHeight(_height);
}

//--------------------------------------------------

void GUIObject::setSize(float _width, int _height)
{
	objMat.setWidth(_width);
	objMat.setHeight(_height);
}

//--------------------------------------------------

void GUIObject::setSize(float _width, float _height)
{
	objMat.setWidth(_width);
	objMat.setHeight(_height);
}

//--------------------------------------------------

void GUIObject::setAlignX(alignTypeX _alignX)
{
	objMat.setAlignX(_alignX);
}

//--------------------------------------------------

void GUIObject::setAlignY(alignTypeY _alignY)
{
	objMat.setAlignY(_alignY);
}

//--------------------------------------------------

void GUIObject::setPivotX(pivotX _pX)
{
	objMat.setPivotX(_pX);
}

//--------------------------------------------------

void GUIObject::setPivotY(pivotY _pY)
{
	objMat.setPivotY(_pY);
}

//--------------------------------------------------

void GUIObject::setViewport(std::vector<glm::vec4*>* _viewports,
		std::vector<glm::mat4*>* _matrixStack)
{
	objMat.setViewport(_viewports->front()); // pixels kalkulationen beziehen sich auf den ersten
											 // viewport, die anderen werden mit relativen koordinaten gezeichnet
	objMat.setMatrixStack(_matrixStack);

	// prüfe ob die anzahl der viewports der anzahl widgGuiMat entspricht
	if (static_cast<short>(_viewports->size())
			!= static_cast<short>(objGuiMat.size()))
		for (short i = 0; i < static_cast<int>(_viewports->size()); i++)
			objGuiMat.push_back(GUIMat());

	// setze die parentSizes der Transformationsmatrizen des Widget anhand des übergebenen ViewportSets
	std::vector<GUIMat>::iterator matIt = objGuiMat.begin();
	int i = 0;
	for (std::vector<glm::vec4*>::iterator it = _viewports->begin();
			it != _viewports->end(); ++it, ++matIt, ++i)
	{
		// handover the parameters from the widgMat to the viewport
		(*matIt).setRefMat(&objMat);
		(*matIt).setViewport(*it);
		(*matIt).setMatrixStack(_matrixStack);

		// std::cout << "GUIObject get TransViewport: " << glm::to_string(*(*matIt).getTransViewport()) << std::endl;

		// set the viewport with the size and offset of the parent widget
		glViewportIndexedf(i, (*matIt).getTransViewport()->x,
				(*matIt).getTransViewport()->y, (*matIt).getTransViewport()->z,
				(*matIt).getTransViewport()->w);
	}

	// std::cout << "GUIObject pushing Matrix: " << glm::to_string( *objMat.getTrans() ) << std::endl;

	// add the new widget Viewports to the viewport stack
	_matrixStack->push_back(objMat.getTrans());

//        setLabelMat();
}

//--------------------------------------------------

void GUIObject::setAnimUpdtQ(std::vector<GUIAnimation*>* _animUpdtQ,
		std::vector<std::vector<GUIAnimation*>::iterator>* _animUpdtQToKill)
{
	animUpdtQ = _animUpdtQ;
	animUpdtQToKill = _animUpdtQToKill;
}

//--------------------------------------------------

void GUIObject::setColor(float r, float g, float b, float a)
{
	color = glm::vec4(r, g, b, a);
}

//--------------------------------------------------

void GUIObject::setHighLightColor(widgetEvent _event, float r, float g, float b,
		float a)
{
	std::map<widgetEvent, GUICallbackFunction>::iterator it = actions.find(
			_event);
	if (it != actions.end())
		highLightCol[_event] = glm::vec4(r, g, b, a);
}

//--------------------------------------------------

void GUIObject::setBackColor(float r, float g, float b, float a)
{
	backColor = glm::vec4(r, g, b, a);
}

//--------------------------------------------------

void GUIObject::setBackTex(std::string path)
{
	hasBackTex = true;
	backTex = new TextureManager();
	backTex->loadTexture2D(path);
}

//--------------------------------------------------

void GUIObject::setId(int* _id)
{
	objId = *_id;
	(*_id)++;
}

//--------------------------------------------------

void GUIObject::setFont(const char* path, int _size)
{
	ftTex = new FreetypeTex(path, _size);
}

//--------------------------------------------------

void GUIObject::setLabel(std::string _label, alignTypeX alignH,
		alignTypeY alignV)
{
	labelAlignX = alignH;
	labelAlignY = alignV;

	if (ftTex && ftTex->ready())
	{
		ftTex->setText(_label);

		label = new Quad(-1.f, -1.f, 2.f, 2.f);
		label->rotate(M_PI, 1.f, 0.f, 0.f);

		setLabelMat();

		hasLabel = true;
	}
}

//--------------------------------------------------

void GUIObject::setLabelMat()
{
	if (static_cast<short>(labelMats.size())
			!= static_cast<short>(objGuiMat.size()))
		for (short i = 0; i < static_cast<int>(objGuiMat.size()); i++)
			labelMats.push_back(glm::mat4(1.f));

	if (ftTex && ftTex->ready())
	{
		std::vector<GUIMat>::iterator vpIt = objGuiMat.begin();
		for (std::vector<glm::mat4>::iterator it = labelMats.begin();
				it != labelMats.end(); ++it, ++vpIt)
		{
			// get proportion of the label
			glm::vec3 propScale = glm::vec3(
					float(ftTex->getWidth()) / (*vpIt).getTransViewport()->z,
					float(ftTex->getHeight()) / (*vpIt).getTransViewport()->w,
					1.f);
			glm::vec3 trans = glm::vec3(0.f);

			switch (labelAlignX)
			{
			case ALIGN_LEFT:
				trans.x = -1.f + propScale.x;
				break;
			case ALIGN_RIGHT:
				trans.x = 1.f - propScale.x;
				break;
			default:
				break;
			}

			switch (labelAlignY)
			{
			case ALIGN_BOTTOM:
				trans.y = -1.f + propScale.y;
				break;
			case ALIGN_TOP:
				trans.y = 1.f - propScale.y;
				break;
			default:
				break;
			}

			if (borderType == PIXELS)
				border = glm::vec4(
						float(iBorder.x) / (*vpIt).getTransViewport()->z,
						float(iBorder.y) / (*vpIt).getTransViewport()->z,
						float(iBorder.z) / (*vpIt).getTransViewport()->w,
						float(iBorder.w) / (*vpIt).getTransViewport()->w) * 2.f;

			trans += glm::vec3(trans.x < 0.f ? border.x : -border.y,
					trans.y < 0.f ? border.w : -border.z, 0.f);

			(*it) = glm::scale(glm::translate(*(*vpIt).getTrans(), trans),
					propScale);
		}
	}
}

//--------------------------------------------------

void GUIObject::setLabelBorder(float left, float right, float top, float bottom)
{
	borderType = PERCENT;
	border = glm::vec4(left, right, top, bottom);
}

//--------------------------------------------------

void GUIObject::setLabelBorder(int left, int right, int top, int bottom)
{
	borderType = PIXELS;
	iBorder = glm::ivec4(left, right, top, bottom);
}

//--------------------------------------------------

void GUIObject::setParentSize(float _width, float _height)
{
	//objMat.setParentSize(_width, _height);
}

//--------------------------------------------------

void GUIObject::setAction(widgetEvent _event, GUICallbackFunction _action)
{
	actions[_event] = _action;
}

//--------------------------------------------------

void GUIObject::setActionAnim(widgetEvent _event, guiAnimFunc _animFunc)
{
	std::map<widgetEvent, GUICallbackFunction>::iterator it = actions.find(
			_event);
	if (it != actions.end())
		actionAnimFuncs[_event] = _animFunc;
}

//--------------------------------------------------

void GUIObject::setActionAnimDur(guiAnimFunc _animFunc, double _dur)
{
	anim.setDuration(_animFunc, _dur);
}

//--------------------------------------------------

void GUIObject::callAction(widgetEvent _event)
{
	std::map<widgetEvent, GUICallbackFunction>::iterator it = actions.find(
			_event);
	if (it != actions.end())
	{
		actions[_event]();
		if (!this->anim.isRunning())
			this->startAnim(_event);
	}
}

//--------------------------------------------------

void GUIObject::removeAnimFromQ()
{
	bool found = false;
	std::vector<GUIAnimation*>::iterator toKill;
	for (std::vector<GUIAnimation*>::iterator it = animUpdtQ->begin();
			it != animUpdtQ->end(); ++it)
		if ((*it) == &anim)
		{
			toKill = it;
			found = true;
			break;
		}

	if (found)
		animUpdtQToKill->push_back(toKill);
}

//--------------------------------------------------

GUIObject::~GUIObject()
{
	if (back)
		delete back;
	if (ftTex)
		delete ftTex;
}
}
