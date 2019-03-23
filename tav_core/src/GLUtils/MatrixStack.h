//
//  MatrixStack.h
//  tav_core
//
//  Created by Sven Hahne on 08/04/16.
//  Copyright © 2016 Sven Hahne. All rights reserved..
//

#ifndef MatrixStack_h
#define MatrixStack_h

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <vector>
#include <utility>

namespace tav
{
class MatrixStack
{
public:
	MatrixStack()
	{
	}
	~MatrixStack()
	{
	}

	void clear()
	{
		stack.clear();
	}

	void push_back(glm::vec2* _parentSize, glm::mat4* _matrix)
	{
		stack.push_back(std::make_pair(_parentSize, _matrix));
	}

	void pop_back()
	{
		stack.pop_back();
	}

	std::vector<std::pair<glm::vec2*, glm::mat4*> > stack;
};
}
#endif /* MatrixStack_h */
