//
//  GUIImgSlider.cpp
//  tav_core
//
//  Created by Sven Hahne on 29/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "GUIImgSlider.h"

namespace tav
{
GUIImgSlider::GUIImgSlider() :
		GUIObject()
{
	init();
}

//--------------------------------------------------

GUIImgSlider::GUIImgSlider(glm::vec2 _pos, glm::vec2 _size) :
		GUIObject(_pos, _size)
{
	init();
}

//--------------------------------------------------

void GUIImgSlider::init()
{
	back = new Quad(-1.f, -1.f, 2.f, 2.f);

	imgPos = glm::vec3(0.f);
	rightOff = glm::vec3(3.f, 0.f, 0.f);
	leftOff = glm::vec3(-3.f, 0.f, 0.f);
	topOff = glm::vec3(0.f, 3.f, 0.f);
	bottOff = glm::vec3(0.f, -3.f, 0.f);
}

//--------------------------------------------------

void GUIImgSlider::draw(std::vector<glm::vec4*>* _viewports,
		std::vector<glm::mat4*>* _matrixStack, Shaders* _guiColShdr,
		Shaders* _guiTexShdr)
{
	setViewport(_viewports, _matrixStack);

	glm::vec4 white = glm::vec4(1.f);

	// widgMatr ist irgendwie altes konzept...
	glm::mat4 actMatr = glm::translate(*objMat.getTrans(), imgPos);

	_guiTexShdr->begin();
	_guiTexShdr->setUniformMatrix4fv("m_pvm", objMat.getTransPtr());
	_guiTexShdr->setUniform1i("tex", 0);
	_guiTexShdr->setUniform4fv("frontColor", &white[0]);

	glActiveTexture(GL_TEXTURE0);
	textures[nextTex]->bind();
	back->draw();

	_guiTexShdr->setUniformMatrix4fv("m_pvm", &actMatr[0][0]);
	glActiveTexture(GL_TEXTURE0);
	textures[actTex]->bind();

	back->draw();
}

//--------------------------------------------------

void GUIImgSlider::startAnim(widgetEvent _event)
{
	printf("guimg start anim \n");

	if (!anim.isRunning())
	{
		switch (_event)
		{
		case SWIPE_RIGHT:
			printf("swipe_right \n");
			anim.setAnimVal<glm::vec3>(actionAnimFuncs[_event], &imgPos,
					&rightOff, [this]()
					{	return this->oneImgForw();});
			break;
		case SWIPE_LEFT:
			printf("swipe_left \n");
//                    anim.setAnimVal<glm::vec3>(actionAnimFuncs[_event], &imgPos, &rightOff,
//                                               [this]() { return this->oneImgForw(); });
			nextTex = (actTex - 1 + nrTex) % nrTex;
			anim.setAnimVal<glm::vec3>(actionAnimFuncs[_event], &imgPos,
					&leftOff, [this]()
					{	return this->oneImgBack();});
			break;
		case SWIPE_UP:
			anim.setAnimVal<glm::vec3>(actionAnimFuncs[_event], &imgPos,
					&topOff, [this]()
					{	return this->oneImgForw();});
			break;
		case SWIPE_DOWN:
			anim.setAnimVal<glm::vec3>(actionAnimFuncs[_event], &imgPos,
					&bottOff, [this]()
					{	return this->oneImgForw();});
//                    nextTex = (actTex -1 + nrTex) % nrTex;
//                    anim.setAnimVal<glm::vec3>(actionAnimFuncs[_event], &imgPos, &bottOff,
//                                               [this]() { return this->oneImgBack(); });
			break;
		default:
			break;
		}

		anim.start();
		// push the animation to the animation queue in Root Widget for optimized updating
		animUpdtQ->push_back(&anim);
	}
}

//--------------------------------------------------

void GUIImgSlider::setTextures(std::vector<std::string> paths)
{
	nrTex = static_cast<int>(paths.size());

	actTex = 0;
	nextTex = (actTex + 1) % nrTex;

	textures = new TextureManager*[nrTex];
	for (int i = 0; i < nrTex; i++)
	{
		textures[i] = new TextureManager();
		textures[i]->loadTexture2D(paths[i]);
	}
}

//--------------------------------------------------

void GUIImgSlider::setActImg(int imgId)
{
	actTex = imgId;
	nextTex = (actTex + 1) % nrTex;
}

//--------------------------------------------------

void GUIImgSlider::oneImgBack()
{
	actTex = nextTex;
	nextTex = (actTex + 1) % nrTex;
	imgPos = glm::vec3(0.f);

	removeAnimFromQ();
}

//--------------------------------------------------

void GUIImgSlider::oneImgForw()
{
	actTex = nextTex;
	nextTex = (actTex + 1) % nrTex;
	imgPos = glm::vec3(0.f);

	removeAnimFromQ();
}

//--------------------------------------------------

GUIImgSlider::~GUIImgSlider()
{
}
}
