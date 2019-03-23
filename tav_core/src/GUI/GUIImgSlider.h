//
//  GUIImgSlider.h
//  tav_core
//
//  Created by Sven Hahne on 29/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <stdio.h>
#include "GUIObject.h"
#include "GeoPrimitives/Quad.h"

namespace tav
{
class GUIImgSlider: public GUIObject
{
public:
	GUIImgSlider();
	GUIImgSlider(glm::vec2 _pos, glm::vec2 _size);
	~GUIImgSlider();

	void init();
	void draw(std::vector<glm::vec4*>* _viewports,
			std::vector<glm::mat4*>* _matrixStack, Shaders* _guiColShdr,
			Shaders* _guiTexShdr);
	void startAnim(widgetEvent _event);
	void setTextures(std::vector<std::string> paths);
	void setActImg(int imgId);

	void oneImgForw();
	void oneImgBack();

private:
	TextureManager** textures;
	int nrTex;
	int actTex;
	int nextTex;
	glm::vec3 imgPos;
	glm::vec3 leftOff;
	glm::vec3 rightOff;
	glm::vec3 topOff;
	glm::vec3 bottOff;
};
}
