/*
*	Cs1PerspBg.cpp
*	tav_gl4
*
*	Created by Sven Hahne on 14.08.14.
*	Copyright (c) 2014 Sven Hahne. All rights reserved.
*
*	Standard CameraSetup, die Kamera befindet bei (0|0|1) und sieht auf den Ursprung
*	sichtbarer Bereich ist ca. von (-1|+1)|(-1|+1)|(0|-100)
*	je nach Seitenverhältnis. Near ist 1.f und Far 100.0f (sichrbarer Bereich beginnt bei z=0)
*
*	Rendert in einen FBO
*	Ergebnis wird über einen Hintergrund gerendert (Render FBO mit Blur)
*	Keine Aspect Korrektur, rendert 1:1
*
*/

#include "pch.h"
#include "Cs1PerspBg.h"

#define STRINGIFY(A) #A

namespace tav
{
Cs1PerspBg::Cs1PerspBg(sceneData* _scd, OSCData* _osc) :
		CameraSet(1, _scd, _osc)
{
	id = _scd->id;

	cam = new GLMCamera*[nrCameras];

	lookAt[0] = glm::vec3(0.f, 0.f, 0.f);
	upVec[0] = glm::vec3(0.f, 1.f, 0.f);
	cam[0] = new GLMCamera(GLMCamera::FRUSTUM, scrWidth, scrHeight,
			-1.0f, 1.0f, -1.0f, 1.0f,                // left, right, bottom, top
			_scd->camPos.x, _scd->camPos.y, _scd->camPos.z, _scd->camLookAt.x,
			_scd->camLookAt.y, _scd->camLookAt.z,   // lookAt
			_scd->camUpVec.x, _scd->camLookAt.y, _scd->camLookAt.z, 1.f, 100.f);

	quad->setColor(0.f, 0.f, 0.f, 0.f);

	fbo = new FBO(_scd->shaderCollector, scrWidth, scrHeight, 8);
	fbo->setMinFilter(GL_LINEAR);
	fbo->setMagFilter(GL_LINEAR);

	fastBlur = new FastBlur(_osc, _scd->shaderCollector, 512, 512, GL_RGBA8);

	stdTex = _scd->shaderCollector->getStdTex();

	setupCamPar();
	initShader();
}

//--------------------------------------------------------------------------------

Cs1PerspBg::~Cs1PerspBg()
{
	delete[] cam;
}

//--------------------------------------------------------------------------------

void Cs1PerspBg::initProto(ShaderProto* _proto)
{
	_proto->defineVert(false);
	if (!_proto->enableShdrType[GEOMETRY])
		_proto->defineGeom(false);
	_proto->defineFrag(false);
	_proto->assemble();
}

//--------------------------------------------------------------------------------

void Cs1PerspBg::render(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	glScissor(0, 0, scrWidth, scrHeight);

	// call update method of the scene, if blending is enabled
	// this already gets called in the scene blender. the resulting morphscene
	// will have a empty update method
	_scene->updateOscPar(time, dt);
	_scene->update(time, dt);

	fbo->bind();
	fbo->clearAlpha(1.f - osc->alpha);

	if (lightProto.find(_scene->protoName) != lightProto.end())
	{
		lightProto[_scene->protoName]->preRender(_scene, time, dt); // pre rendering e.g. for shadow maps
		lightProto[_scene->protoName]->shader->begin();
		lightProto[_scene->protoName]->sendPar(1, cp, osc, time);   // 1 cameras

		cp.camId = id;
		cp.actFboSize = glm::vec2(float(scrWidth), float(scrHeight));

		_scene->draw(time, dt, &cp, lightProto[_scene->protoName]->shader);

	}
	else
	{
		printf("CameraSet Error: couldn´t get LightProto \n");
	}

	fbo->unbind();
}

//--------------------------------------------------------------------------------

void Cs1PerspBg::renderFbos(double time, double dt, unsigned int ctxNr)
{
	fastBlur->proc(fbo->getColorImg());

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	// render background with effect
	bgShader->begin();
	bgShader->setIdentMatrix4fv("m_pvm");
	bgShader->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fastBlur->getResult());
	quad->draw();

	// render fbo as is
	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fbo->getColorImg());
	quad->draw();
}

//--------------------------------------------------------------------------------

void Cs1PerspBg::initShader()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=1) in vec3 normal; layout (location=2) in vec2 texCoord; layout (location=3) in vec4 color; out vec2 tex_coord; void main(void) { tex_coord = texCoord * 0.25 + vec2(0.2, 0.2); gl_Position = position; });
	vert = "// Cs1PerspBg add shape Vert\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					in vec2 tex_coord; uniform sampler2D tex; layout(location = 0) out vec4 color; void main() { color = texture(tex, tex_coord) * 0.8; });
	frag = "// Cs1PerspBg add shape frag\n" + shdr_Header + frag;

	bgShader = shCol->addCheckShaderText("Cs1PerspBg_Back", vert.c_str(),
			frag.c_str());
}

//--------------------------------------------------------------------------------

void Cs1PerspBg::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs1PerspBg::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs1PerspBg::onKey(int key, int scancode, int action, int mods)
{
	// call onKey Function of active SceneNode
}

//--------------------------------------------------------------------------------

void Cs1PerspBg::mouseBut(GLFWwindow* window, int button, int action, int mods)
{
}

//--------------------------------------------------------------------------------

void Cs1PerspBg::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
}

//--------------------------------------------------------------------------------

void Cs1PerspBg::clearFbo()
{
}
}
