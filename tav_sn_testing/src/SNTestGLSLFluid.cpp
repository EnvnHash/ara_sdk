//
//  SNGLSLFluidTest.cpp
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "SNTestGLSLFluid.h"

namespace tav
{

SNTestGLSLFluid::SNTestGLSLFluid(sceneData* _scd, std::map<std::string, float>* _sceneArgs) : SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	scrScale = 2;
	flWidth = _scd->screenWidth / scrScale;
	flHeight = _scd->screenHeight / scrScale;

	addPar("mouseX", &mouseX);
	addPar("mouseY", &mouseY);

	forceScale = glm::vec2(static_cast<float>(flWidth) * 0.5f, static_cast<float>(flHeight) * 0.85f);

	circle = new Circle(30, 0.1f, 0.f);
	circle->translate(0.f, -0.25f, 0.f);

	colShader = shCol->getStdCol();

	fluidSim = new GLSLFluid(false, shCol);
	fluidSim->allocate(flWidth, flHeight, 0.5f);

	// Seting the gravity set up & injecting the background image
	fluidSim->dissipation = 0.99f;
	fluidSim->velocityDissipation = 0.99f;
	fluidSim->setGravity(glm::vec2(0.0f,0.0f));

	//  Set obstacle
	fluidSim->begin();
	colShader->begin();
	colShader->setIdentMatrix4fv("m_pvm");
	circle->draw();
	colShader->end();
	fluidSim->end();

	// Adding constant forces
	fluidSim->addConstantForce(forceScale,
			glm::vec2(0.f, -2.f),
			glm::vec4(0.5f, 0.1f, 0.0f, 1.f),
			10.f);
}

//----------------------------------------------------

SNTestGLSLFluid::~SNTestGLSLFluid()
{
	fluidSim->cleanUp();
	delete fluidSim;
	delete circle;
	delete colShader;
}

//----------------------------------------------------

void SNTestGLSLFluid::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	// ---update---
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
	fluidSim->update();
	fluidSim->draw();
	//        fluidSim->drawVelocity();
}

//----------------------------------------------------

void SNTestGLSLFluid::update(double time, double dt)
{
}

//----------------------------------------------------

void SNTestGLSLFluid::onKey(int key, int scancode, int action, int mods)
{
	/*
        if (action == GLFW_PRESS)
        {
            switch (key)
            {
                case GLFW_KEY_1 :
                    break;
            }
        }
	 */
}

//----------------------------------------------------

void SNTestGLSLFluid::onCursor(double xpos, double ypos)
{
//	mouseX = xpos;
//	mouseY = ypos;
}

}
