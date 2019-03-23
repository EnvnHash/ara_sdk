//
//  Cs3InRow.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  CameraSetup, für drei Bildschirme, die sich auf gleicher Höhe
//  nebeneinander befinden, bzw. drei Beamer mit Overlap
//  sichtbarer Bereich ist ca. von (-1|+1)|(-1|+1)|(0|-100)
//  je nach Seitenverhältnis. Near ist 0.1f und Far 100.0f
//  scrWidth und scrHeight sollten sich auf die Groesse beider Bildschirme
//  zusammen beziehen
//

#include "pch.h"
#include "Cs3InRow.h"

namespace tav
{
Cs3InRow::Cs3InRow(sceneData* _scd, OSCData* _osc) :
		CameraSet(3, _scd, _osc)
{
	id = _scd->id;

//        overlap = 0.1f;
	overlap = 0.000f;

	lookAt[0] = glm::vec3(0.f, 0.f, 0.f);

	float sWidth = 2.f / 3.f;
	float maxOverlap = 2.f / 6.f;

	cam = new GLMCamera*[nrCameras];
	for (int i = 0; i < nrCameras; i++)
	{
		float fInd = static_cast<float>(i);
		cam[i] = new GLMCamera(GLMCamera::FRUSTUM, scrWidth / nrCameras,
				scrHeight,
				std::fmax(-1.0f + sWidth * fInd - overlap * maxOverlap, -1.f), // left
				std::fmin(-1.0f + sWidth * (fInd + 1.0f) + overlap * maxOverlap,
						1.f), // right
				-1.0f, 1.0f,                             // bottom, top
				0.f, 0.f, 1.f,                           // camPos
				lookAt[0].x, lookAt[0].y, lookAt[0].z);   // lookAt

	}

	setupCamPar();
}

Cs3InRow::~Cs3InRow()
{
}

//--------------------------------------------------------------------------------

void Cs3InRow::initProto(ShaderProto* _proto)
{
	_proto->defineVert(true);
	if (!_proto->enableShdrType[GEOMETRY])
		_proto->defineGeom(true);
	_proto->defineFrag(true);

	_proto->asmblMultiCamGeo(); // add geo shader for multicam rendering
	_proto->add3RowsEdgeBlend(); // add edge blending
	_proto->enableShdrType[GEOMETRY] = true;

	_proto->assemble();
}

//--------------------------------------------------------------------------------
/*
 void Cs3InRow::clearScreen()
 {
 glViewportIndexedf(0, 0.f, 0.f, scrWidth, scrHeight);
 
 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

 quad->setColor(0.f, 0.f, 0.f, 1.f - osc->feedback),

 glClear(GL_DEPTH_BUFFER_BIT);   // necessary, sucks performance
 
 colShader->begin();
 fullScrCam->sendIdentMtr(colShader->getProgram(), "m_pvm");
 
 glDepthMask(GL_FALSE);
 glDisable(GL_DEPTH_TEST);
 quad->draw();
 glDepthMask(GL_TRUE);
 glEnable(GL_DEPTH_TEST);
 
 colShader->end();
 }
 */
//--------------------------------------------------------------------------------
void Cs3InRow::render(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	// call update method of the scene, if blending is enabled
	// this already gets called in the scene blender. the resulting morphscene
	// will have a empty update method
	_scene->updateOscPar(time, dt);
	_scene->update(time, dt);

	if (lightProto.find(_scene->protoName) != lightProto.end())
	{
		lightProto[_scene->protoName]->preRender(_scene, time, dt);
		lightProto[_scene->protoName]->shader->begin();
		lightProto[_scene->protoName]->sendPar(nrCameras, cp, osc, time);

		// set viewports
		const float wot = float(scrWidth) / 3.f;
		glViewportIndexedf(0, 0.f, 0.f, wot, scrHeight);
		glViewportIndexedf(1, wot, 0.f, wot, scrHeight);
		glViewportIndexedf(2, wot * 2.f, 0.f, wot, scrHeight);

		lightProto[_scene->protoName]->shader->setUniform1f("scrWidth",
				scrWidth);

		float overL = overlap != 0.f ? 1.f / (overlap * 2.f) : 10000.f;
		lightProto[_scene->protoName]->shader->setUniform1f("overlapV", overL);
		cp.camId = id;

		_scene->draw(time, dt, &cp, lightProto[_scene->protoName]->shader);
	}
	else
	{
		printf("CameraSet Error: couldn´t get LightProto \n");
	}
}

//--------------------------------------------------------------------------------

void Cs3InRow::renderFbos(double time, double dt, unsigned int ctxNr)
{
}

//--------------------------------------------------------------------------------

void Cs3InRow::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs3InRow::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs3InRow::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_O:
			overlap += 0.01f;
			std::cout << "overlap: " << overlap << std::endl;
			break;
		case GLFW_KEY_L:
			overlap -= 0.01f;
			std::cout << "overlap: " << overlap << std::endl;
			break;
		}
	}
}

//--------------------------------------------------------------------------------

void Cs3InRow::mouseBut(GLFWwindow* window, int button, int action, int mods)
{
}

//--------------------------------------------------------------------------------

void Cs3InRow::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
}

//--------------------------------------------------------------------------------

void Cs3InRow::clearFbo()
{
}
}
