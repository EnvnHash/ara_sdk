//
//  Cs1PerspFboFft.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 22.12.15.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Standard CameraSetup, die Kamera befindet sich ein wenig
//  "ausserhalb des Bilderschirms", und sieht auf den Ursprung
//  sichtbarer Bereich ist ca. von (-1|+1)|(-1|+1)|(0|-100)
//  je nach Seitenverhältnis. Near ist 0.1f und Far 100.0f
//
#define STRINGIFY(A) #A

#include "pch.h"
#include "Cs1PerspFboFft.h"

namespace tav
{
Cs1PerspFboFft::Cs1PerspFboFft(sceneData* _scd, OSCData* _osc, std::vector<fboView*>* _fboViews,
		GWindowManager* _winMan, OSCHandler* _osc_handler) :
		CameraSet(1, _scd, _osc), winMan(_winMan), osc_handler(_osc_handler)
{
	id = _scd->id;
	sendCtr = 0;
	sendInt = 1;
	gridRowSize = 10;
	gridSize = gridRowSize * gridRowSize;
	fftSize = 512;
	gridStep = float(fftSize) / float(gridRowSize);
	halfGridStep = gridStep * 0.5f;

	cam = new GLMCamera*[nrCameras];

	lookAt[0] = glm::vec3(0.f, 0.f, 0.f);
	upVec[0] = glm::vec3(0.f, 1.f, 0.f);
	cam[0] = new GLMCamera(GLMCamera::FRUSTUM, scrWidth, scrHeight, -1.f, 1.0f,
			-1.0f, 1.0f,                // left, right, bottom, top
			0.f, 0.f, 1.f,                           // camPos
			lookAt[0].x, lookAt[0].y, lookAt[0].z,   // lookAt
			upVec[0].x, upVec[0].y, upVec[0].z, 1.f, 100.f);

	quad->setColor(0.f, 0.f, 0.f, 0.f);

	setupCamPar();
	setupFunc = std::bind(&CameraSet::setupCamPar, this, true);

	fboDedist = new FboDedistPersp(_scd, _winMan, _scd->fboWidth,
			_scd->fboHeight);
	fboDedist->setCamSetupFunc(&setupFunc);

	stdTex = _scd->shaderCollector->getStdTex();

//	fft = new FFT(fftSize);

	renderFbo = new PingPongFbo(_scd->shaderCollector, gridRowSize, gridRowSize,
			GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	diffFbo = new FBO(_scd->shaderCollector, gridRowSize, gridRowSize, GL_RGBA8,
			GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);

	// make a bufer for donwloading the fbo
	data = new unsigned char[gridRowSize * gridRowSize * 4];
	gridVals = new float[gridSize];
	outAmp = new float[gridSize];
	outHue = new float[gridSize];

	medHues = new Median<float>*[gridSize];
	medAmps = new Median<float>*[gridSize];
	for (int i = 0; i < gridSize; i++)
	{
		medAmps[i] = new Median<float>(10.f);
		medHues[i] = new Median<float>(10.f);
	}

	initDiffShdr();
}

//--------------------------------------------------------------------------------

void Cs1PerspFboFft::initProto(ShaderProto* _proto)
{
	_proto->defineVert(false);
	if (!_proto->enableShdrType[GEOMETRY])
		_proto->defineGeom(false);
	_proto->defineFrag(false);
	_proto->assemble();
}

//--------------------------------------------------------------------------------

void Cs1PerspFboFft::render(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	// check if the fboViews changed and update if necessary
	for (std::vector<fboView*>::iterator it = osc->fboViews->begin();
			it != osc->fboViews->end(); ++it)
	{
		if ((*it)->update && (*it)->srcCamId == id)
		{
			fboDedist->updtFboView(it);
			(*it)->update = false;
		}
	}

	//  glScissor(0,0, scrWidth, scrHeight);

	// call update method of the scene, if blending is enabled
	// this already gets called in the scene blender. the resulting morphscene
	// will have a empty update method
	_scene->updateOscPar(time, dt);
	_scene->update(time, dt);

	if (lightProto.find(_scene->protoName) != lightProto.end())
	{
		lightProto[_scene->protoName]->preRender(_scene, time, dt); // pre rendering e.g. for shadow maps

		// ------ render to FBO -------------

		fboDedist->bindFbo();
		//fboDedist->clearFboAlpha(osc->feedback, osc->backColor);
		//fboDedist->clearFbo(winMan->getWindows()->at(0));

		lightProto[_scene->protoName]->shader->begin();
		lightProto[_scene->protoName]->sendPar(1, cp, osc, time);   // 1 cameras

		cp.camId = id;
		cp.actFboSize = glm::vec2(float(scrWidth), float(scrHeight));

		_scene->draw(time, dt, &cp, lightProto[_scene->protoName]->shader);

		fboDedist->unbindFbo();
	}
	else
	{
		printf("CameraSet Error: couldn´t get LightProto \n");
	}
}

//--------------------------------------------------------------------------------

void Cs1PerspFboFft::renderFbos(double time, double dt, unsigned int ctxNr)
{
	/*
	 glClear(GL_DEPTH_BUFFER_BIT);   // necessary, sucks performance

	 glScissor(0,0, scrWidth, scrHeight);
	 glViewportIndexedf(0,       0.f, 0.f, f_scrWidth, f_scrHeight);

	 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	 glDepthMask(GL_FALSE);

	 clearCol.r = osc->backColor;
	 clearCol.g = osc->backColor;
	 clearCol.b = osc->backColor;
	 clearCol.a =  1.f - osc->feedback;

	 clearShader->begin();
	 clearShader->setIdentMatrix4fv("m_pvm");
	 clearShader->setUniform4fv("clearCol", &clearCol[0], 1);
	 quad->draw();

	 glDepthMask(GL_TRUE);
	 */

	// ------ render with perspective Dedistortion -------------
	fboDedist->drawAllFboViews();

	/*
	 glClear(GL_COLOR_BUFFER_BIT);

	 //----------------------

	 stdTex->begin();
	 stdTex->setIdentMatrix4fv("m_pvm");
	 stdTex->setUniform1i("tex", 0);

	 glActiveTexture(GL_TEXTURE0);
	 glBindTexture(GL_TEXTURE_2D, diffFbo->getColorImg());

	 quad->draw();
	 */

	// ----------- render the dedist Fbo one time into the fft fbo -------------------
	if (sendCtr % sendInt == 0)
	{
		// --------- render to fbo ----------------------

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		renderFbo->dst->bind();
		renderFbo->dst->clearAlpha(0.7f);

		stdTex->begin();
		stdTex->setIdentMatrix4fv("m_pvm");
		stdTex->setUniform1i("tex", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fboDedist->getFboTex(0));

		quad->draw();

		renderFbo->dst->unbind();
		renderFbo->swap();

		// -------- render diff---------------------------

		diffFbo->bind();
		diffFbo->clear();

		diffShdr->begin();
		diffShdr->setIdentMatrix4fv("m_pvm");
		diffShdr->setUniform1i("tex", 0);
		diffShdr->setUniform1i("lastTex", 1);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, renderFbo->dst->getColorImg(0));

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, renderFbo->src->getColorImg(0));

		quad->draw();

		diffFbo->unbind();

		//---- download fbo----------------------------------------------------

		glBindTexture(GL_TEXTURE_2D, diffFbo->getColorImg());
		glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, &data[0]);

		// ----------- send to sc ----------------------------------

		//	memset(gridVals, 0, gridSize);

		// sample a grid of brightnesses
		for (int y = 0; y < gridRowSize; y++)
		{
			for (auto x = 0; x < gridRowSize; x++)
			{
				int gridOffs = y * gridRowSize + x;

				glm::vec3 col = glm::vec3(float(data[gridOffs * 4]) / 256.f,
						float(data[gridOffs * 4 + 1]) / 256.f,
						float(data[gridOffs * 4 + 2]) / 256.f);
				glm::vec3 hsv = RGBtoHSV(col.r, col.g, col.b);

				gridVals[gridOffs] = std::min(
						std::sqrt(
								glm::dot(col,
										glm::vec3(0.2126, 0.7152, 0.0722))),
						1.f);

				medAmps[gridOffs]->update(gridVals[gridOffs] * 4.f);
				if (std::isfinite(hsv.r))
					medHues[gridOffs]->update(hsv.r);
				else
					medHues[gridOffs]->update(0.f);

				//std::cout << medAmps[gridOffs]->get() << std::endl;

				outAmp[gridOffs] = std::min(
						std::max(medAmps[gridOffs]->get(), 0.f), 1.f);
				outHue[gridOffs] = std::min(
						std::max(medHues[gridOffs]->get(), 0.f), 1.f);
			}
		}

		// do fft
//		fft->setSignal(oneRow);
//		fft->preparePolar();
//		float* amps = fft->getAmplitude();
//		for (int x=0;x<fftSize/2;x++)
//		{
//			medAmps[x]->update(amps[x]);
//			outAmp[x] = medAmps[x]->get();
//		}
////			std::cout << " [" << x << "]:" << amps[x] << std::cout;
//		std::cout << std::endl;

		// send to SuperCollider
		sendCtr = 0;

		osc_handler->sendFFT("127.0.0.1", "57120", "/mags", outAmp, gridSize);
		osc_handler->sendFFT("127.0.0.1", "57120", "/hues", outHue, gridSize);

		osc_handler->sendFFT("169.254.0.2", "57120", "/mags", outAmp, gridSize);
		osc_handler->sendFFT("169.254.0.2", "57120", "/hues", outHue, gridSize);
	}

	sendCtr++;
}

//--------------------------------------------------------------------------------

void Cs1PerspFboFft::initDiffShdr()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=1) in vec3 normal; layout (location=2) in vec2 texCoord; layout (location=3) in vec4 color; uniform mat4 m_pvm; out vec2 tex_coord; void main(void) { tex_coord = texCoord; gl_Position = m_pvm * position; });
	vert = "// Cs1PerspFboFft diff shader\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 color; uniform sampler2D tex; uniform sampler2D lastTex; in vec2 tex_coord; void main() {
					//color = texture(lastTex, tex_coord);
					color = vec4( texture(tex, tex_coord).rgb - texture(lastTex, tex_coord).rgb, 1.0); });
	frag = "// Cs1PerspFboFft diff shader\n" + shdr_Header + frag;

	diffShdr = shCol->addCheckShaderText("Cs1PerspFboFft_diff", vert.c_str(),
			frag.c_str());

}

// r,g,b values are from 0 to 1
glm::vec3 Cs1PerspFboFft::RGBtoHSV(float r, float g, float b)
{
	glm::vec3 out;
	double min, max, delta;

	r *= 100.f;
	g *= 100.f;
	b *= 100.f;

	min = r < g ? r : g;
	min = min < b ? min : b;

	max = r > g ? r : g;
	max = max > b ? max : b;

	out.b = max;                                // v
	delta = max - min;
	if (max > 0.0)
	{ // NOTE: if Max is == 0, this divide would cause a crash
		out.g = (delta / max);                  // s
	}
	else
	{
		// if max is 0, then r = g = b = 0
		// s = 0, v is undefined
		out.g = 0.0;
		out.r = NAN;                            // its now undefined
		return out;
	}
	if (r >= max)                       // > is bogus, just keeps compilor happy
		out.r = (g - b) / delta;        // between yellow & magenta
	else if (g >= max)
		out.r = 2.0 + (b - r) / delta;  // between cyan & yellow
	else
		out.r = 4.0 + (r - g) / delta;  // between magenta & cyan

	out.r *= 60.0;                              // degrees

	if (out.r < 0.0)
		out.r += 360.0;

	out.r /= 360.f;
	out.g /= 100.f;
	out.b /= 100.f;

	return out;
}

//--------------------------------------------------------------------------------

void Cs1PerspFboFft::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs1PerspFboFft::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs1PerspFboFft::onKey(int key, int scancode, int action, int mods)
{
	// call onKey Function of active SceneNode
	fboDedist->onKey(key, scancode, action, mods);
}

//--------------------------------------------------------------------------------

void Cs1PerspFboFft::mouseBut(GLFWwindow* window, int button, int action,
		int mods)
{
	fboDedist->mouseBut(window, button, action, mods);
}

//--------------------------------------------------------------------------------

void Cs1PerspFboFft::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
	fboDedist->mouseCursor(window, xpos, ypos);
}

//--------------------------------------------------------------------------------

Cs1PerspFboFft::~Cs1PerspFboFft()
{
	delete[] cam;
}

//--------------------------------------------------------------------------------

void Cs1PerspFboFft::clearFbo()
{
	fboDedist->clearFbo();
}
}
