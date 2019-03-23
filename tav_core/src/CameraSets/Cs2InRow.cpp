//
//  Cs2InRow.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  CameraSetup, für zwei Bildschirme, die sich auf gleicher Höhe
//  nebeneinander befinden, bzw. zwei Beamer mit Overlap
//  sichtbarer Bereich ist ca. von (-1|+1)|(-1|+1)|(0|-100)
//  je nach Seitenverhältnis. Near ist 0.1f und Far 100.0f
//  scrWidth und scrHeight sollten sich auf die Groesse beider Bildschirme
//  zusammen beziehen
//

#include "pch.h"
#include "Cs2InRow.h"

namespace tav
{
Cs2InRow::Cs2InRow(sceneData* _scd, OSCData* _osc) :
		CameraSet(2, _scd, _osc)
{
	id = _scd->id;
	overlap = 0.081f;

	cam = new GLMCamera*[nrCameras];
	cam[0] = new GLMCamera(GLMCamera::FRUSTUM, scrWidth / 2, scrHeight, -1.0f,
			1.0f * overlap, -1.0f, 1.0f,      // left, right, bottom, top
			0.f, 0.f, 1.f,                          // camPos
			0.f, 0.f, -1.f);  // lookAt

	cam[1] = new GLMCamera(GLMCamera::FRUSTUM, scrWidth / 2, scrHeight,
			-1.0f * overlap, 1.0f, -1.0f, 1.0f,      // left, right, bottom, top
			0.f, 0.f, 1.f,                           // camPos
			0.f, 0.f, -1.f);   // lookAt

	setupCamPar();
}

Cs2InRow::~Cs2InRow()
{
}

//--------------------------------------------------------------------------------

void Cs2InRow::initProto(ShaderProto* _proto)
{
	_proto->defineVert(true);
	if (!_proto->enableShdrType[GEOMETRY])
		_proto->defineGeom(true);
	_proto->defineFrag(true);

	_proto->asmblMultiCamGeo(); // add geo shader for multicam rendering
	_proto->add2RowsEdgeBlend(); // add edge blending
	_proto->enableShdrType[GEOMETRY] = true;

	_proto->assemble();
}

//--------------------------------------------------------------------------------
/*
 void Cs2InRow::clearScreen()
 {
 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

 quad->setColor(0.f, 0.f, 0.f, 1.f - osc->feedback),

 glViewportIndexedf(0,       0.f, 0.f, scrWidth, scrHeight);

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
void Cs2InRow::render(SceneNode* _scene, double time, double dt,
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
		const float wot = float(scrWidth) * 0.5f;
		glViewportIndexedf(0, 0.f, 0.f, wot, scrHeight);
		glViewportIndexedf(1, wot, 0.f, wot, scrHeight);
		glScissor(0.0, 0.0, scrWidth, scrHeight);

		lightProto[_scene->protoName]->shader->setUniform1f("scrWidth", scrWidth);

		float overL = overlap != 0.f ? 1.f / (overlap * 0.55f) : 10000.f;
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

void Cs2InRow::renderFbos(double time, double dt, unsigned int ctxNr)
{
}

//--------------------------------------------------------------------------------
/*
 // andere variante statt im fragment shader zu blenden, ist aber langsamer...
 void Cs2InRow::textureEdgeBlend()
 {
 glViewportIndexedf(0, 0.f, 0.f, scrWidth, scrHeight);
 
 // wenn overlap, render noch weiche verläufe auf die kanten
 if (overlap > 0.f)
 {
 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

 texShader->begin();
 fullScrCam->sendIdentMtr(texShader->getProgram(), "m_pvm");

 glDisable(GL_DEPTH_TEST);

 tex.bind(0, 0, 0);
 for (auto i=0;i<nrCameras;i++) softEdgeQuads[i]->draw();
 tex.unbind();

 texShader->end();
 }
 //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
 }
 */
//--------------------------------------------------------------------------------
void Cs2InRow::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs2InRow::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs2InRow::onKey(int key, int scancode, int action, int mods)
{
}

//--------------------------------------------------------------------------------

void Cs2InRow::mouseBut(GLFWwindow* window, int button, int action, int mods)
{
}

//--------------------------------------------------------------------------------

void Cs2InRow::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
}

//--------------------------------------------------------------------------------

void Cs2InRow::clearFbo()
{
}

}
