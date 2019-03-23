//
//  FastBlur.cpp
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "FastBlur.h"

#define STRINGIFY(A) #A

namespace tav
{
FastBlur::FastBlur(OSCData* _osc, ShaderCollector* _shCol, int _blurW,
		int _blurH, GLenum _type) :
		blurW(_blurW), blurH(_blurH), osc(_osc), shCol(_shCol), useOsc(true), type(
				_type)
{
	fAlpha = 1.0f;
	offsScale = 1.f;
	blFdbk = 0.4f;

	init();
}

//----------------------------------------------------

FastBlur::FastBlur(ShaderCollector* _shCol, int _blurW, int _blurH,
		GLenum _type) :
		blurW(_blurW), blurH(_blurH), shCol(_shCol), useOsc(false), type(_type)
{
	fAlpha = 1.0f;
	offsScale = 1.0f;
	blFdbk = 0.4f;

	init();
}

//----------------------------------------------------

void FastBlur::init()
{
	fboQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f,
			0.f, 0.f, 1.f);

	initShader();

	fWidth = static_cast<float>(blurW);
	fHeight = static_cast<float>(blurH);

	blurOffsV = new GLfloat[3];
	blurOffsV[0] = 0.f;
	blurOffsV[1] = 0.00135216346152f;
	blurOffsV[2] = 0.00315504807695f;

	blurOffsH = new GLfloat[3];
	blurOffsH[0] = 0.f;
	blurOffsH[1] = 0.00135216346152f;
	blurOffsH[2] = 0.00315504807695f;

	pp = new PingPongFbo(shCol, blurW, blurH, type, GL_TEXTURE_2D, false, 1, 1,
			1, GL_CLAMP_TO_EDGE);

	switch (type)
	{
	case GL_RGBA8:
		data = new unsigned char[blurW * blurH * 4];
		break;
	case GL_R16:
		data16 = new unsigned short[blurH * blurW];
		break;
	default:
		data = new unsigned char[blurW * blurH * 4];
		break;
	}
}

//----------------------------------------------------

FastBlur::~FastBlur()
{
	delete fboQuad;
	pp->cleanUp();
	delete pp;
	linearH->remove();
	delete linearH;
	linearV->remove();
	delete linearV;

	delete[] blurOffsH;
	delete[] blurOffsV;
}

//----------------------------------------------------

void FastBlur::proc(GLint texIn)
{
	glEnable(GL_BLEND);

	if (useOsc)
	{
		blurOffsV[1] = 0.00135216346152f * osc->blurOffs;
		blurOffsV[2] = 0.00315504807695f * osc->blurOffs;
	}
	else
	{
		blurOffsV[1] = 0.00135216346152f;
		blurOffsV[2] = 0.00315504807695f;
	}

	pp->dst->bind();

	if (useOsc)
		pp->dst->clearAlpha(osc->blurFboAlpha, osc->backColor);
	else
		pp->dst->clearAlpha(0.f, 0.f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	// linear vertical blur
	linearV->begin();
	linearV->setUniform1i("image", 0);
	linearV->setUniform1i("old", 1);
	linearV->setUniform1f("width", fWidth);
	linearV->setUniform1f("height", fHeight);
	linearV->setUniform1fv("offset", &blurOffsV[0], 3);

	if (useOsc)
	{
		linearV->setUniform1f("oldFdbk", osc->blurFdbk);
		linearV->setUniform1f("newFdbk", std::fmax(1.f - osc->blurFdbk, 0.f));
	}
	else
	{
		linearV->setUniform1f("oldFdbk", blFdbk);
		linearV->setUniform1f("newFdbk", 1.f - blFdbk);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texIn);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pp->getSrcTexId());

	fboQuad->draw();
	// linearV->end();

	pp->dst->unbind();
	pp->swap();

	// linear horizontal blur
	pp->dst->bind();

	if (useOsc)
		pp->dst->clearAlpha(osc->blurFboAlpha, osc->backColor);
	else
		pp->dst->clearAlpha(0.f, 0.f);

	// glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	if (useOsc)
	{
		blurOffsH[1] = 0.00135216346152f * osc->blurOffs;
		blurOffsH[2] = 0.00315504807695f * osc->blurOffs;
	}
	else
	{
		blurOffsH[1] = 0.00135216346152f;
		blurOffsH[2] = 0.00315504807695f;
	}

	linearH->begin();
	linearH->setUniform1i("image", 0);
	linearH->setUniform1f("width", fWidth);
	linearH->setUniform1f("bright", 1.f);
	linearH->setUniform1f("height", fHeight);
	linearH->setUniform1fv("offset", &blurOffsH[0], 3);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pp->getSrcTexId());
	fboQuad->draw();
	linearH->end();

	pp->dst->unbind();
	pp->swap();
}

//----------------------------------------------------

void FastBlur::downloadData()
{
	switch (type)
	{
	case GL_RGBA8:
		glBindTexture(GL_TEXTURE_2D, pp->getSrcTexId());
		glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, &data[0]);
		break;
	case GL_R16:
		glBindTexture(GL_TEXTURE_2D, pp->getSrcTexId());
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_SHORT, &data16[0]);
		break;
	default:
		glBindTexture(GL_TEXTURE_2D, pp->getSrcTexId());
		glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, &data[0]);
		break;
	}
}

//----------------------------------------------------

void FastBlur::initShader()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 2 ) in vec2 texCoord; out vec2 tex_coord; void main(void) { tex_coord = texCoord; gl_Position = vec4(position.xy, 0.0, 1.0); });

	vert = "// FastBlur vertex shader\n" + shdr_Header + vert;
	std::string frag =
			STRINGIFY(
					uniform sampler2D image; uniform sampler2D old;

					uniform float width; uniform float height; uniform float oldFdbk; uniform float newFdbk;

					in vec2 tex_coord; out vec4 FragmentColor;

					uniform float offset[3]; float weight[3] = float[]( 0.2270270270, 0.3162162162, 0.0702702703 );

					void main(void){ FragmentColor = (texture(image,tex_coord) *newFdbk + texture(old,tex_coord)*oldFdbk) * weight[0]; if (FragmentColor.a < 0.001) discard; for (int i=1; i<3; i++) { FragmentColor += (texture(image, tex_coord+vec2(0.0, offset[i])) * newFdbk + texture(old, tex_coord+vec2(0.0, offset[i])) * oldFdbk) * weight[i]; FragmentColor += (texture(image, tex_coord-vec2(0.0, offset[i])) * newFdbk +texture(old, tex_coord-vec2(0.0, offset[i])) * oldFdbk) * weight[i]; } });

	frag = "// FastBlur Vertical fragment shader\n" + shdr_Header + frag;

	linearV = shCol->addCheckShaderText("FastBlurVShader", vert.c_str(),
			frag.c_str());

	//----------------------------------------------------

	frag =
			STRINGIFY(uniform sampler2D image;

			uniform float width; uniform float height; uniform float bright;

			in vec2 tex_coord; out vec4 FragmentColor;

			uniform float offset[3];
			//uniform float offset[3] = float[]( 0.0, 1.3846153846, 3.2307692308 );
					float weight[3] = float[]( 0.2270270270, 0.3162162162, 0.0702702703 );

					void main(void){ FragmentColor = texture( image, tex_coord ) * weight[0];

					for (int i=1; i<3; i++) { FragmentColor += texture( image, tex_coord +vec2(offset[i], 0.0) ) * weight[i]; FragmentColor += texture( image, tex_coord -vec2(offset[i], 0.0) ) * weight[i]; } FragmentColor = FragmentColor * bright; });

	frag = "// FastBlur horizontal fragment shader\n" + shdr_Header + frag;

	linearH = shCol->addCheckShaderText("FastBlurHShader", vert.c_str(),
			frag.c_str());
}

//----------------------------------------------------

GLint FastBlur::getResult()
{
	return pp->getSrcTexId();
}

//----------------------------------------------------

unsigned char* FastBlur::getData()
{
	return &data[0];
}

//----------------------------------------------------

unsigned short* FastBlur::getDataR16()
{
	return &data16[0];
}
}
