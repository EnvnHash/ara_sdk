//
//  SceneNodeBlender.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 26.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SceneNodeBlender.h"

namespace tav
{
SceneNodeBlender::SceneNodeBlender(SetupLoader* _sl) :
		sl(_sl)
{
	cam = _sl->getCamera()->front();
	maxNrSceneNodes = sl->getNrSceneNodes();
	oscd = sl->getOscData();

	debug = false;
	scbData = new scBlendData();
	scenes = new SceneNode*[2];
	oldSceneNodes = new SceneNode*[2];
	for (auto i = 0; i < 2; i++)
		oldSceneNodes[i] = nullptr;

	//- Setup Shaders ---

	recOnlyShaders = new Shaders*[2];
	for (auto i = 0; i < 2; i++)
	{
		//recOnlyShaders[i] = new Shaders("shaders/std_rec.vert", "shaders/std_rec.frag", true);
		recOnlyShaders[i] = _sl->shaderCollector->getStdRec();
	}

	//- Setup XFO ---

	std::vector<std::string> names;
	// copy the standard record attribute names, number depending on hardware
	for (auto i = 0; i < MAX_SEP_REC_BUFS; i++)
		names.push_back(stdRecAttribNames[i]);

	geoRecorders = new TFO*[2];

	for (int i = 0; i < 2; i++)
	{
		geoRecorders[i] = new TFO();
		geoRecorders[i]->setVaryingsToRecord(&names,
				recOnlyShaders[i]->getProgram());
		glLinkProgram(recOnlyShaders[i]->getProgram());
		geoRecorders[i]->setVaryingsNames(names);
	}

	//- testing -

	quad = new Quad();
	quad->translate(0.f, 0.f, -1.f);

	/*
	 morphSceneNode = new SNMorphSceneNode(sl->getSceneData(0),
	 oscd,
	 geoRecorders,
	 scbData,
	 cam->cp.multicam_model_matrix,
	 cam->cp.multicam_view_matrix,
	 cam->cp.multicam_projection_matrix);
	 */
	sl->getWinMan()->addKeyCallback(0,
			[this](int key, int scancode, int action, int mods)
			{
				return morphSceneNode->onKey(key, scancode, action, mods);});

}

//---------------------------------------------------------------

SceneNodeBlender::~SceneNodeBlender()
{
}

//---------------------------------------------------------------

void SceneNodeBlender::blend(double time, double dt)
{
	procOsc();

	// berechne alles, was mit den tfos kollidieren würde
	for (auto i = 0; i < 2; i++)
		scenes[i]->update(time, dt);

	// -- berechne beide szenen und nimm sie in getrennte TFOs auf 

	glEnable(GL_RASTERIZER_DISCARD);

	for (auto i = 0; i < 2; i++)
	{
		recOnlyShaders[i]->begin();
		recOnlyShaders[i]->setUniformMatrix4fv("modelMatrix",
				cam->cp.multicam_model_matrix);
		recOnlyShaders[i]->setUniformMatrix4fv("projectionMatrix",
				cam->cp.multicam_projection_matrix);

		// render scene 1
		geoRecorders[i]->bind();
		geoRecorders[i]->begin(GL_TRIANGLES);

		scenes[i]->draw(time, dt, &cam->cp, recOnlyShaders[i], geoRecorders[i]);

		geoRecorders[i]->end();
		geoRecorders[i]->unbind();

		recOnlyShaders[i]->end();
	}

	glDisable(GL_RASTERIZER_DISCARD);

	// hand over the shaderProto of the source scene to the morphScene
	morphSceneNode->setShaderProtoName(
			sl->sceneNodes[fromSource]->getShaderProtoName());

}

//---------------------------------------------------------------

void SceneNodeBlender::procOsc()
{
	scNum1 = std::min(oscd->sceneNum1, maxNrSceneNodes - 1);
	scNum2 = std::min(oscd->sceneNum2, maxNrSceneNodes - 1);

	if (debug)
		std::cout << "einstieg prepare mapping: scNum1: " << scNum1
				<< "  scNum2: " << scNum2 << std::endl;
	if (debug)
		std::cout << "einstieg prepare mapping: fromSource: " << fromSource
				<< std::endl;

	// bestimme den parameter selektor
	if (oscd->scBlend <= 0.0001f)
	{
		fromSource = scNum1;
		fromSourceTook = 0;
	}

	if (oscd->scBlend >= 0.9999f)
	{
		fromSource = scNum2;
		fromSourceTook = 1;
	}

	if (fromSource == -1)
	{
		if (oscd->scBlend > 0.5f)
		{
			fromSource = scNum2;
			fromSourceTook = 1;
		}
		else
		{
			fromSource = scNum1;
			fromSourceTook = 0;
		}
	}

	if (fromSourceTook == 0 && fromSource != scNum1)
		fromSource = scNum1;
	if (fromSourceTook == 1 && fromSource != scNum2)
		fromSourceTook = scNum2;

	if (debug)
		std::cout << "bestimme den parameter selektor: fromSource: "
				<< fromSource << std::endl;

	fromSource = static_cast<int>(fminf(static_cast<float>(fromSource),
			static_cast<float>(maxNrSceneNodes - 1)));

	if (fromSource == scNum1)
	{
		toSource = scNum2;
		relBlend = oscd->scBlend;
	}
	else
	{
		toSource = scNum1;
		relBlend = 1.0f - oscd->scBlend;
	}
	if (relBlend < 0.0001f)
		relBlend = 0.0f;

	scbData->relBlend = relBlend;

	scenes[0] = sl->sceneNodes[fromSource];
	scenes[1] = sl->sceneNodes[toSource];

	for (auto i = 0; i < 2; i++)
		oldSceneNodes[i] = scenes[i];

	//cout << "scene 1: " << scenes[0] << " scenes 2:" << scenes[1] << std::endl;
	if (debug)
		std::cout << "  toSource " << toSource << " relBlend: " << relBlend
				<< std::endl;
}

//---------------------------------------------------------------

void SceneNodeBlender::setSnapShotMode(bool _val)
{
	morphSceneNode->setSnapShotMode(_val);
}

//---------------------------------------------------------------

SceneNode* SceneNodeBlender::getFromSceneNode()
{
	return sl->sceneNodes[fromSource];
}

//---------------------------------------------------------------

SceneNode* SceneNodeBlender::getToSceneNode()
{
	return sl->sceneNodes[toSource];
}

//---------------------------------------------------------------

SceneNode* SceneNodeBlender::getSceneNode0()
{
	return sl->sceneNodes[std::min(oscd->sceneNum1, sl->nrSceneNodes - 1)];
}

//---------------------------------------------------------------

SceneNode* SceneNodeBlender::getMorphSceneNode()
{
	return morphSceneNode;
}
}
