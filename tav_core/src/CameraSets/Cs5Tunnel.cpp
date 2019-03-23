//
//  Cs5Tunnel.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  tunnel mit 5 projektore - 2 an allen waenden + boden led bildschirm
//  es werden 3 kameras fuer die waende und eine fuer den boden gesetzt
//	diese werden mit den realen proportionen (frustrum) der leinwaende gerendert
//	im format 16/9 ausgegeben (viewport verzerrt) und anschliessend per fbodedist
//	wieder in die richtige Proportion skaliert
//
//        2
//    ---------
//   |         |         Y  x
//   |         |         ^  ^
// 1 |         | 3        \ |
//   |         |           \|
//   -         -            ----> z
//   |         |
//   |         |
// 0 |         | 4
//   |         |
//
//
// die 5 screens haben alle gleiche Groesse und liegen alle horizontal nebeneinander
// auflösung  bestimmt sich durch das xml-Setup (width = width /5, height=height

#define STRINGIFY(A) #A

#include "Cs5Tunnel.h"

namespace tav
{
Cs5Tunnel::Cs5Tunnel(sceneData* _scd, OSCData* _osc, GWindowManager* _winMan) :
		CameraSet(5, _scd, _osc), winMan(_winMan), renderMode(DRAW)
{
	id = _scd->id;

	cam = new GLMCamera*[nrCameras];

	// alle werte werden in den norm cubus skaliert
	tunnelDepth = _scd->roomDim->x * 0.001f;     // referenz = 2.f, x wert
	tunnelHeight = _scd->roomDim->y * 0.001f;    // y groesse
	tunnelWidth = _scd->roomDim->z * 0.001f;      // z groesse

	// skaliere tunnel dimensionen auf normkubus
	// bestimme den skalierungsfaktor
	float scaleF = 2.f / tunnelDepth;

	// skaliere
	tunnelHeight = tunnelHeight * scaleF;	// 0,628
	tunnelWidth = tunnelWidth * scaleF;	// 0,628
	tunnelDepth = 2.f;

	lookAt[0] = glm::vec3(0.f, 0.f, -1.f);
	lookAt[1] = glm::vec3(0.f, 0.f, -1.f);
	lookAt[2] = glm::vec3(1.f, 0.f, 0.f);
	lookAt[3] = glm::vec3(0.f, 0.f, 1.f);
	lookAt[4] = glm::vec3(0.f, 0.f, 1.f);

	// mit dem richtigen aspect (der leiunwaende) skalieren und danach in was auch
	// immer fuer eine Aufloesung rendern (wird verzerrt)
	// dann mit distortfbo korrigieren
	// tunnelDepth ist die Referenz, entspricht 1.f
	float widths[5] = { tunnelDepth, tunnelDepth, tunnelWidth, tunnelDepth, tunnelDepth };
	float nears[5] = { tunnelWidth * 0.5f, tunnelWidth * 0.5f, tunnelDepth * 0.5f, tunnelWidth
			* 0.5f, tunnelWidth * 0.5f };
	float lefts[5] = { tunnelDepth * -0.5f, 0.f, tunnelWidth * -0.5f, tunnelDepth * -0.5f, 0.f };
	float rights[5] = { 0.f, tunnelDepth * 0.5f, tunnelWidth * 0.5f, 0.f, tunnelDepth * 0.5f };
	float tops[5] = { tunnelHeight * 0.5f, tunnelHeight * 0.5f, tunnelHeight * 0.5f,
			tunnelHeight * 0.5f, tunnelHeight * 0.5f };

	for (int i = 0; i < 5; i++)
		cam[i] = new GLMCamera(GLMCamera::FRUSTUM, int(10000.f * widths[i]),
				int(tunnelHeight * 10000.f), lefts[i], rights[i], // left, right
				-tops[i], tops[i],   // bottom, top
				0.f, 0.f, 0.f,	// camPos
				lookAt[i].x, lookAt[i].y, lookAt[i].z,	// lookAt
				0.f, 1.f, 0.f, nears[i], 100.f);  	// upVec
	setupCamPar();
	cp.near = tunnelWidth * 0.5f;

	// first render all 3 cameras (3 sides of the tunnel) into one fbo
	// then render each wall and back with two fbos
	// all screens exist as one xscreen (xinerama)
	fboDedist = new FboDedistPersp(_scd, _winMan, _scd->fboWidth, _scd->fboHeight);

	for (int i = 0; i < nrCameras; i++)
		fboDedist->add(_winMan->getWindows()->at(0), cam[i],
				float(scd->screenWidth / nrCameras), float(scd->screenHeight),
				float(i * scd->screenWidth / nrCameras), 0);

	setupFunc = std::bind(&CameraSet::setupCamPar, this, true);
	fboDedist->setCamSetupFunc(&setupFunc);
	fboDedist->setCalibFileName( (*_scd->dataPath) + "/calib_cam/" + _scd->setupName + "_Cs5Tunnel.yml");

	if (renderMode == VISU)
	{
		tunVisQuads = new Quad*[5];

		for (auto i = 0; i < 4; i++)
			tunVisQuads[i < 2 ? i : i + 1] = new Quad(-0.5f,
					-tunnelHeight * 0.5f, 1.f, tunnelHeight,
					glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f, 0.f, 1.f);
		tunVisQuads[2] = new Quad(-tunnelWidth * 0.5f, -tunnelHeight * 0.5f,
				tunnelWidth, tunnelHeight, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
				0.f, 1.f);

		tunVisQuads[0]->translate(-0.5f, 0.f, -tunnelWidth * 0.5f);	// links vorne
		tunVisQuads[1]->translate(0.5f, 0.f, -tunnelWidth * 0.5f); // links hinten

		tunVisQuads[2]->translate(0.f, 0.f, tunnelDepth * -0.5f);	// hinten
		tunVisQuads[2]->rotate(M_PI * -0.5f, 0.f, 1.f, 0.f);

		tunVisQuads[3]->translate(-0.5f, 0.f, -tunnelWidth * 0.5f);	// rechts hinten
		tunVisQuads[3]->rotate(M_PI, 0.f, 1.f, 0.f);

		tunVisQuads[4]->translate(0.5f, 0.f, -tunnelWidth * 0.5f); // rechts vorne
		tunVisQuads[4]->rotate(M_PI, 0.f, 1.f, 0.f);

		tunVisCam = new GLMCamera(GLMCamera::PERSPECTIVE, _scd->screenWidth,
				_scd->screenHeight, -1.f, 1.f,  		 // left, right
				-1.f, 1.f,      	 // bottom, top
				0.f, 0.f, 1.f,	// camPos
				0.f, 0.f, 0.f,	// lookAt
				0.f, 1.f, 0.f, 0.5f, 100.f, 0.34f * 180.f);  	// upVec

		glm::mat4 modMatr = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 0.32f - 1.f));
		modMatr = glm::rotate(modMatr, float(M_PI) * 0.47f, glm::vec3(0.f, 1.f, 0.f));

		tunVisCam->setModelMatr(modMatr);

		//    	floorTex = new TextureManager();
		//    	floorTex->loadTexture2D((*scd->dataPath) + "textures/floor_text.jpg");
	}

	initShader();

	patron = new TextureManager();
	patron->loadTexture2D((*scd->dataPath) + "textures/tv.jpg");

	patronQuad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f,
			0.f, 0.f, 1.f);
}

//--------------------------------------------------------------------------------

Cs5Tunnel::~Cs5Tunnel()
{
}

//--------------------------------------------------------------------------------

void Cs5Tunnel::initShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color; uniform mat4 m_pvm; uniform vec2 tSize; uniform vec2 tOffs; uniform vec2 fboSize; out vec2 tex_coord;

					void main() { tex_coord = texCoord * (tSize / fboSize) + (tOffs / fboSize); gl_Position = m_pvm * position; });

	vert = "// Cs5Tunnel Visu, vert\n" + shdr_Header + vert;

	// zero is left top
	std::string frag =
			STRINGIFY(
					uniform sampler2D tex;

					in vec2 tex_coord; layout (location = 0) out vec4 color; void main() { color = texture(tex, tex_coord);
//    						color = texture(tex, vec2(distTexCoord.x * texMod.x + texMod.z,
//    												  distTexCoord.y * texMod.y + texMod.w));
					});

	frag = "//  Cs5Tunnel Visu, frag\n" + shdr_Header + frag;

	texShdr = shCol->addCheckShaderText("Cs5TunnelVisu", vert.c_str(),
			frag.c_str());
}

//--------------------------------------------------------------------------------

void Cs5Tunnel::initProto(ShaderProto* _proto)
{
	_proto->defineVert(true);
	if (!_proto->enableShdrType[GEOMETRY])
		_proto->defineGeom(true);
	_proto->defineFrag(true);

	_proto->asmblMultiCamGeo(); // add geo shader for multicam rendering
	_proto->enableShdrType[GEOMETRY] = true;

	_proto->assemble();
}

//--------------------------------------------------------------------------------

void Cs5Tunnel::clearFbo()
{
	fboDedist->bindFbo();
	fboDedist->clearFbo();
	fboDedist->unbindActFbo();
}

//--------------------------------------------------------------------------------

void Cs5Tunnel::render(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	// call update method of the scene, if blending is enabled
	// this already gets called in the scene blender. the resulting morphscene
	// will have a empty update method
	_scene->updateOscPar(time, dt);
	_scene->update(time, dt);

	// ------ render to FBO -------------

	if (lightProto.find(_scene->protoName) != lightProto.end())
	{
		fboDedist->bindFbo();

		lightProto[_scene->protoName]->shader->begin();
		lightProto[_scene->protoName]->sendPar(nrCameras, cp, osc, time);

		const float wot = float(scd->screenWidth / nrCameras);

		// set viewports
		for (short i = 0; i < nrCameras; i++)
			glViewportIndexedf(i, wot * float(i), 0.f, wot, scd->screenHeight); // 2 screens left

		// die groesse des FBO in den gezeichnet wird, sollte der groesse
		// des screens im setup xml entsprechen
		cp.actFboSize = fboDedist->getFboSize(0);
		cp.camId = id;

		lightProto[_scene->protoName]->sendPar(nrCameras, cp, osc, time); // 1 cameras
		_scene->draw(time, dt, &cp, lightProto[_scene->protoName]->shader);

		fboDedist->unbindActFbo();
	}
	else
	{
		printf("CameraSet Error: couldn´t get LightProto \n");
	}
}

//--------------------------------------------------------------------------------

void Cs5Tunnel::renderFbos(double time, double dt, unsigned int ctxNr)
{
	// ------ render with perspective Dedistortion each part -------------
	/*
	 delete tunVisCam;

	 tunVisCam = new GLMCamera(
	 GLMCamera::PERSPECTIVE,
	 scd->screenWidth,
	 scd->screenHeight,
	 -1.f, 1.f,  		 // left, right
	 -1.f, 1.f,      	 // bottom, top
	 0.f, 0.f, 1.f,	// camPos
	 0.f, 0.f, 0.f,	// lookAt
	 0.f, 1.f, 0.f,
	 0.5f, 100.f, osc->zoom * 180.f);  	// upVec
	 */

	//glm::mat4 modMatr = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, osc->blurOffs - 1.f));
	//modMatr = glm::rotate(modMatr, float(M_PI) * osc->blurFdbk, glm::vec3(0.f, 1.f, 0.f));
	//tunVisCam->setModelMatr(modMatr);
	switch (renderMode)
	{
	case DRAW:
		fboDedist->drawAllFboViews();
		break;

	case VISU:
		glActiveTexture(GL_TEXTURE0);

		glClearDepth(1.0f);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		texShdr->begin();
		texShdr->setUniformMatrix4fv("m_pvm", tunVisCam->getMVPPtr());
		texShdr->setUniform1i("tex", 0);

		for (auto i = 0; i < nrCameras; i++)
		{
			glBindTexture(GL_TEXTURE_2D, fboDedist->getFboTex(i));

			texShdr->setUniform2fv("tSize", &fboDedist->getScreenSize(i)[0]);
			texShdr->setUniform2fv("tOffs", &fboDedist->getScreenOffs(i)[0]);
			texShdr->setUniform2fv("fboSize", &fboDedist->getFboSize(i)[0]);

			tunVisQuads[i]->draw();
		}

		glEnable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		break;

	case TESTPIC:
		fboDedist->drawTestPic();
		break;
	}
}

//--------------------------------------------------------------------------------

void Cs5Tunnel::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs5Tunnel::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs5Tunnel::onKey(int key, int scancode, int action, int mods)
{
	// call onKey Function of active SceneNode
	fboDedist->onKey(key, scancode, action, mods);
}

//--------------------------------------------------------------------------------

void Cs5Tunnel::mouseBut(GLFWwindow* window, int button, int action, int mods)
{
	fboDedist->mouseBut(window, button, action, mods);
}

//--------------------------------------------------------------------------------

void Cs5Tunnel::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
	fboDedist->mouseCursor(window, xpos, ypos);
}
}
