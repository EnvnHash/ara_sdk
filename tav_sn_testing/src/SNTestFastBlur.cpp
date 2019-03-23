//
//  SNTestFastBlur.cpp
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "SNTestFastBlur.h"

namespace tav
{
SNTestFastBlur::SNTestFastBlur(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs)
{
	osc = static_cast<OSCData*>(scd->osc);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	fAlpha = 1.0f;
	offsScale = 1.25f;
	blFdbk = 0.5f;

	fboQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	quad = scd->stdQuad;
	quad->rotate(M_PI, 0.f, 0.f, 1.f);
	quad->rotate(M_PI, 0.f, 1.f, 0.f);


	linearV = new Shaders("shaders/passthrough.vs", "shaders/linear_vert.fs", true);
	linearV->link();

	linearH = new Shaders("shaders/passthrough.vs", "shaders/linear_horiz.fs", true);
	linearH->link();

	shdr = new Shaders("shaders/basic_tex.vert", "shaders/basic_tex.frag", true);
	shdr->link();

	blurW = 512;
	blurH = 512;
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

	pp = new PingPongFbo(shCol, blurW, blurH, GL_RGBA8, GL_TEXTURE_2D);
}

//----------------------------------------------------

SNTestFastBlur::~SNTestFastBlur()
{
	delete quad;
	delete fboQuad;
	pp->cleanUp();
	delete pp;
	shdr->remove();
	delete shdr;
	linearH->remove();
	delete linearH;
	linearV->remove();
	delete linearV;

	delete [] blurOffsH;
	delete [] blurOffsV;
}

//----------------------------------------------------

void SNTestFastBlur::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{

	blurOffsV[1] = 0.00135216346152f * osc->blurOffs;
	blurOffsV[2] = 0.00315504807695f * osc->blurOffs;

	pp->dst->bind();
	pp->dst->clearAlpha(osc->blurFboAlpha);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	// linear vertical blur
	linearV->begin();
	linearV->setUniform1i("image", 0);
	linearV->setUniform1i("old", 1);
	linearV->setUniform1f("width", fWidth);
	linearV->setUniform1f("height", fHeight);
	linearV->setUniform1fv("offset", &blurOffsV[0], 3);
	linearV->setUniform1f("oldFdbk", osc->blurFdbk);
	linearV->setUniform1f("newFdbk", 1.f - osc->blurFdbk);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pp->getSrcTexId());

	fboQuad->draw();
	linearV->end();

	pp->dst->unbind();

	pp->swap();

	// linear horizontal blur
	pp->dst->bind();
	pp->dst->clearAlpha(osc->blurFboAlpha);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	blurOffsH[1] = 0.00135216346152f *osc->blurOffs;
	blurOffsH[2] = 0.00315504807695f *osc->blurOffs;

	linearH->begin();
	linearH->setUniform1i("image", 0);
	linearH->setUniform1f("width", fWidth);
	linearH->setUniform1f("height", fHeight);
	linearH->setUniform1fv("offset", &blurOffsH[0], 3);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pp->getSrcTexId());
	fboQuad->draw();
	linearH->end();

	pp->dst->unbind();

	pp->swap();

	// show result
	shdr->begin();
	shdr->setIdentMatrix4fv("m_pvm");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pp->getSrcTexId());

	quad->draw();
	shdr->end();
}

//----------------------------------------------------

void SNTestFastBlur::update(double time, double dt)
{
}

//----------------------------------------------------

void SNTestFastBlur::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_1 :
			break;
		case GLFW_KEY_2 :
			break;
		}
	}
}
}
