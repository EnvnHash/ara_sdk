//
//  SNGLSLFluidTest.cpp
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "SNTestGLSLFluid3D.h"

namespace tav
{
SNTestGLSLFluid3D::SNTestGLSLFluid3D(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	scrScale = 1;
	flWidth = _scd->screenWidth / scrScale;
	flHeight = _scd->screenHeight / scrScale;

	forceScale = glm::vec2(static_cast<float>(flWidth) * 0.5f, static_cast<float>(flHeight) * 0.85f);

	circle = new Circle(30, 0.1f, 0.f);
	circle->translate(0.f, -0.25f, 0.f);

	colShader = shCol->getStdCol();

	fluidSim = new GLSLFluid3D(shCol);
}

//----------------------------------------------------

void SNTestGLSLFluid3D::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	/*
        // Adding temporal Force, in pixel relative to flWidth and flHeight
        glm::vec2 m = glm::vec2(mouseX / static_cast<float>(scrScale),
                                mouseY / static_cast<float>(scrScale));
        glm::vec2 d = (m - oldM) * 10.f;
        oldM = m;
        glm::vec2 c = glm::normalize(forceScale - m);

        float tm = static_cast<float>(std::sin(time));
        fluidSim->addTemporalForce(m,                           // pos
                                   d,                           // vel
                                   glm::vec4(std::fabs(c.x * tm),
                                             std::fabs(c.y * tm),
                                             std::fabs(0.5f * tm),
                                             1.f),
                                   3.0f);
        //        fluidSim->drawVelocity();
	 */
	fluidSim->draw();
}

//----------------------------------------------------

void SNTestGLSLFluid3D::update(double time, double dt)
{
	glDisable(GL_CULL_FACE);
	fluidSim->update(time);
}

//----------------------------------------------------

void SNTestGLSLFluid3D::onKey(int key, int scancode, int action, int mods)
{}

//----------------------------------------------------

void SNTestGLSLFluid3D::onCursor(double xpos, double ypos)
{
	mouseX = xpos;
	mouseY = ypos;
}

//----------------------------------------------------

SNTestGLSLFluid3D::~SNTestGLSLFluid3D()
{
	delete fluidSim;
	delete circle;
	delete colShader;
}
}
