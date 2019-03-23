//
//  GUIMat.hpp
//  tav_core
//
//  Created by Sven Hahne on 07/04/16.
//  Copyright © 2016 Sven Hahne. All rights reserved..
//

#ifndef GUIMat_hpp
#define GUIMat_hpp

#pragma once

#include <stdio.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

namespace tav
{
enum unitType
{
	PIXELS = 0, PERCENT = 1
};
enum alignTypeX
{
	CENTER_X = 0, ALIGN_LEFT = 1, ALIGN_RIGHT = 2
};
enum alignTypeY
{
	CENTER_Y = 0, ALIGN_TOP = 1, ALIGN_BOTTOM = 2
};
enum pivotX
{
	PX_LEFT = 0, PX_RIGHT = 1, PX_CENTER = 2
};
enum pivotY
{
	PY_BOTTOM = 0, PY_TOP = 1, PY_CENTER = 2
};

class GUIMat
{
public:
	GUIMat()
	{
	}
	~GUIMat()
	{
	}

	void setX(int _x)
	{
		posXInt = _x;
		posXType = PIXELS;
		changed = true;
	}
	void setY(int _y)
	{
		posYInt = _y;
		posYType = PIXELS;
		changed = true;
	}
	void setX(float _x)
	{
		posXFloat = _x;
		posXType = PERCENT;
		changed = true;
	}
	void setY(float _y)
	{
		posYFloat = _y;
		posYType = PERCENT;
		changed = true;
	}

	void setWidth(int _x)
	{
		widthInt = _x;
		widthType = PIXELS;
		changed = true;
	}
	void setHeight(int _y)
	{
		heightInt = _y;
		heightType = PIXELS;
		changed = true;
	}
	void setWidth(float _x)
	{
		widthFloat = _x;
		widthType = PERCENT;
		changed = true;
	}
	void setHeight(float _y)
	{
		heightFloat = _y;
		heightType = PERCENT;
		changed = true;
	}

	void setAlignX(alignTypeX _type)
	{
		alignX = _type;
		changed = true;
	}
	void setAlignY(alignTypeY _type)
	{
		alignY = _type;
		changed = true;
	}

	void setPivotX(pivotX _type)
	{
		pivX = _type;
		changed = true;
	}
	void setPivotY(pivotY _type)
	{
		pivY = _type;
		changed = true;
	}

	void setRefMat(GUIMat* _ref)
	{
		ref = _ref;
		useRef = true;
	}

//        void setParentSize(float _width, float _height)
//        {
//            parentSize = glm::vec2(_width, _height);
//            changed = true;
//        }

	void setViewport(glm::vec4* _viewport)
	{
		inViewport = _viewport;
		// parentSize = glm::vec2(_viewport->b, _viewport->a);
		changed = true;
	}

	void setMatrixStack(std::vector<glm::mat4*>* _matrixStack)
	{
		matrixStack = _matrixStack;
	}

	void calcTrans()
	{
		// proc matrixStack
		if (matrixStack != 0 && (int) matrixStack->size() > 0
				&& inViewport != 0)
		{
			parentMat = glm::mat4(1.f);
			for (std::vector<glm::mat4*>::iterator it = matrixStack->begin();
					it != matrixStack->end(); ++it)
				parentMat = *(*it) * parentMat;

			procViewPort = parentMat * (*inViewport);
//                std::cout << "inViewport: " << glm::to_string( *inViewport ) << std::endl;
			//std::cout << "procViewPort: " << glm::to_string( procViewPort ) << std::endl;

			if (useRef && ref != 0)
				calcTransRef(ref, true);
			else
				calcTransRef(this, false);
		}
	}

	void calcTransRef(GUIMat* _src, bool _byRef)
	{
		// calculate normalized size
		if (_src->widthType == PIXELS)
		{
			relSize.x = static_cast<float>(_src->widthInt) / procViewPort.z
					* 2.f;
			_src->widthFloat = relSize.x;
			_src->widthType = PERCENT;
		}
		else
			relSize.x = _src->widthFloat;

		if (_src->heightType == PIXELS)
		{
			relSize.y = static_cast<float>(_src->heightInt) / procViewPort.w
					* 2.f;
			_src->heightFloat = relSize.x;
			_src->heightType = PERCENT;
		}
		else
			relSize.y = _src->heightFloat;

		// calculate normalized position horizontal
		switch (_src->alignX)
		{
		case ALIGN_LEFT:
			relPos.x = -1.f;
			break;
		case ALIGN_RIGHT:
			relPos.x = 1.f;
			break;
		case CENTER_X:
			relPos.x = 0.f;
			break;
		}

		switch (_src->pivX)
		{
		case PX_LEFT:
			relPos.x += relSize.x * 0.5f;
			break;
		case PX_RIGHT:
			relPos.x -= relSize.x * 0.5f;
			break;
		case PX_CENTER:
			break;
		}

		if (_src->posXType == PIXELS)
		{
			relPos.x += (static_cast<float>(_src->posXInt) / procViewPort.z)
					* 2.f;
			_src->posXFloat = (static_cast<float>(_src->posXInt)
					/ procViewPort.z) * 2.f;
			_src->posXType = PERCENT;
		}
		else
			relPos.x += _src->posXFloat;

		// calculate normalized position horizontal
		switch (_src->alignY)
		{
		case ALIGN_TOP:
			relPos.y = 1.f;
			break;
		case ALIGN_BOTTOM:
			relPos.y = -1.f;
			break;
		case CENTER_Y:
			relPos.y = 0.f;
			break;
		}

		switch (_src->pivY)
		{
		case PY_TOP:
			relPos.y -= relSize.y * 0.5f;
			break;
		case PY_BOTTOM:
			relPos.y += relSize.y * 0.5f;
			break;
		case PY_CENTER:
			break;
		}

		if (_src->posYType == PIXELS)
		{
			relPos.y += (static_cast<float>(_src->posYInt) / procViewPort.w)
					* 2.f;
			_src->posYFloat = (static_cast<float>(_src->posYInt)
					/ procViewPort.w) * 2.f;
			_src->posYType = PERCENT;
		}
		else
			relPos.y += _src->posYFloat;

		// bezieht sich auf Standard Norm Quad (-1, -1, 2, 2)
		transMat = glm::translate(glm::mat4(1.f),
				glm::vec3(relPos.x, relPos.y, 0.f))
				* glm::scale(glm::mat4(1.f),
						glm::vec3(relSize.x * 0.5f, relSize.y * 0.5f, 1.f));

		outViewport = transMat * parentMat * *inViewport;
		//std::cout << "outViewport: " << glm::to_string( outViewport ) << std::endl;

		changed = false;
	}

	float* getTransPtr()
	{
		if (changed)
			calcTrans();
		return &transMat[0][0];
	}

	glm::mat4* getTrans()
	{
		if (changed)
			calcTrans();
		return &transMat;
	}

	glm::vec2* getRelSize()
	{
		if (changed)
			calcTrans();
		return &relSize;
	}

	glm::vec2* getPos()
	{
		if (posXType == PIXELS)
			outPos.x = (static_cast<float>(posXInt) / procViewPort.w) * 2.f;
		else
			outPos.x = posXFloat;

		if (posYType == PIXELS)
			outPos.y = (static_cast<float>(posYInt) / procViewPort.y) * 2.f;
		else
			outPos.y = posYFloat;

		return &outPos;
	}

	alignTypeX getAlignX()
	{
		return alignX;
	}

	alignTypeY getAlignY()
	{
		return alignY;
	}

	pivotX getPivotX()
	{
		return pivX;
	}

	pivotY getPivotY()
	{
		return pivY;
	}

	glm::vec4* getTransViewport()
	{
		if (changed)
			calcTrans();
		return &outViewport;
	}

	int posXInt = 0;
	int posYInt = 0;
	float posXFloat = 0;
	float posYFloat = 0;

	int widthInt = 1;
	int heightInt = 1;
	float widthFloat = 1;
	float heightFloat = 1;

	unitType posXType = PIXELS;
	unitType posYType = PIXELS;
	unitType widthType = PIXELS;
	unitType heightType = PIXELS;

	alignTypeX alignX = CENTER_X;
	alignTypeY alignY = CENTER_Y;

private:
	bool changed = true;
	bool useRef = false;

	GUIMat* ref = 0;
	//glm::vec2       parentSize;

	glm::vec2 relPos;
	glm::vec2 outPos;
	glm::vec2 relSize;
	glm::vec4 viewPort = glm::vec4(1.f);
	glm::vec4 procViewPort = glm::vec4(1.f);
	glm::vec4 outViewport = glm::vec4(1.f);
	glm::vec4* inViewport = 0;

	glm::mat4 parentMat = glm::mat4(1.f);
	glm::mat4 transMat = glm::mat4(1.f);
	std::vector<glm::mat4*>* matrixStack = 0;

	pivotX pivX = PX_CENTER;
	pivotY pivY = PY_CENTER;
};
}

#endif /* GUIMat_hpp */
