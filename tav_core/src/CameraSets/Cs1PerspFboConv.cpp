//
//  Cs1PerspFboConv.cpp
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

#include "pch.h"
#include "Cs1PerspFboConv.h"

#define STRINGIFY(A) #A

namespace tav
{
Cs1PerspFboConv::Cs1PerspFboConv(sceneData* _scd, OSCData* _osc, std::vector<fboView*>* _fboViews,
		GWindowManager* _winMan) :
		CameraSet(1, _scd, _osc), winMan(_winMan)
{
	id = _scd->id;
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

	fboDedist = new FboDedistPersp(_scd, _winMan, _scd->fboWidth, _scd->fboHeight);
	fboDedist->setCamSetupFunc(&setupFunc);

	convFbo = new FBO(_scd->shaderCollector, _scd->fboWidth, _scd->fboHeight,
			GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);

	initShader();
}

//--------------------------------------------------------------------------------

void Cs1PerspFboConv::initProto(ShaderProto* _proto)
{
	_proto->defineVert(false);
	if (!_proto->enableShdrType[GEOMETRY])
		_proto->defineGeom(false);
	_proto->defineFrag(false);
	_proto->assemble();
}

//--------------------------------------------------------------------------------

void Cs1PerspFboConv::render(SceneNode* _scene, double time, double dt,
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
		fboDedist->clearFboAlpha(osc->feedback, osc->backColor);
		//fboDedist->clearFbo(winMan->getWindows()->at(0));

		lightProto[_scene->protoName]->shader->begin();
		lightProto[_scene->protoName]->sendPar(1, cp, osc, time);   // 1 cameras

		cp.camId = id;
		cp.actFboSize = glm::vec2(float(scd->fboWidth), float(scd->fboHeight));

		_scene->draw(time, dt, &cp, lightProto[_scene->protoName]->shader);

		fboDedist->unbindFbo();

	}
	else
	{
		printf("CameraSet Error: couldn´t get LightProto \n");
	}
}

//--------------------------------------------------------------------------------

void Cs1PerspFboConv::renderFbos(double time, double dt, unsigned int ctxNr)
{
	// render preRender Fbos again with convolution

	convFbo->bind();
	convFbo->clear();

	convShdr->begin();
	convShdr->setUniform1i("image", 0);
	convShdr->setUniform1f("convAmt", osc->blurFdbk);
	convShdr->setUniform1f("normAmt", osc->blurFboAlpha);
	convShdr->setUniform1f("scaleX", osc->blurOffs);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fboDedist->getFboTex(0));

	quad->draw();

	convShdr->end();
	convFbo->unbind();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, convFbo->getFbo());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboDedist->getFbo());
	glBlitFramebuffer(0, 0, scd->fboWidth, scd->fboHeight, 0, 0, scd->fboWidth,
			scd->fboHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ------ render with perspective Dedistortion -------------

	fboDedist->drawAllFboViews();
}

//----------------------------------------------------

void Cs1PerspFboConv::initShader()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 2 ) in vec2 texCoord; out vec2 tex_coord; uniform float scaleX; void main(void) { tex_coord = vec2(texCoord.x * scaleX + (1.0 - scaleX) * 0.5, texCoord.y); gl_Position = vec4(position.xy, 0.0, 1.0); });

	vert = "// FastBlur vertex shader\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					uniform sampler2D image; uniform sampler2D old; uniform float convAmt; uniform float normAmt;

					in vec2 tex_coord; out vec4 FragmentColor;

					vec4 get_pixel(in vec2 coords, in float dx, in float dy) { return texture(image, coords + vec2(dx, dy)); }

					float Convolve(in float[9] kernel, in float[9] matrix, in float denom, in float offset) { float res = 0.0; for (int i=0; i<9; i++) { res += kernel[i]*matrix[i]; } return clamp(res/denom + offset,0.0,1.0); }

					float[9] GetData(in int channel) { float dxtex = 1.0 / float(textureSize(image,0)); float dytex = 1.0 / float(textureSize(image,0)); float[9] mat; int k = -1; for (int i=-1; i<2; i++) { for(int j=-1; j<2; j++) { k++; mat[k] = get_pixel(tex_coord, float(i)*dxtex, float(j)*dytex)[channel]; } } return mat; }

					float[9] GetMean(in float[9] matr, in float[9] matg, in float[9] matb) { float[9] mat; for (int i=0; i<9; i++) { mat[i] = (matr[i]+matg[i]+matb[i])/3.; } return mat; }

					void main(void){ float[9] kerEdgeDetect = float[] (-1./8.,-1./8.,-1./8., -1./8., 1., -1./8., -1./8., -1./8., -1./8.); float matr[9] = GetData(0); float matg[9] = GetData(1); float matb[9] = GetData(2); float mata[9] = GetMean(matr,matg,matb);

					FragmentColor = vec4(Convolve(kerEdgeDetect,mata,0.1,0.), Convolve(kerEdgeDetect, mata,0.1,0.), Convolve(kerEdgeDetect, mata,0.1,0.), 1.0) * convAmt + texture(image, tex_coord) * normAmt; });

	frag = "// Cs1 Persp Convolution fragment shader\n" + shdr_Header + frag;

	convShdr = shCol->addCheckShaderText("Cs1PerspFboConv", vert.c_str(),
			frag.c_str());
}

//--------------------------------------------------------------------------------

void Cs1PerspFboConv::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs1PerspFboConv::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs1PerspFboConv::onKey(int key, int scancode, int action, int mods)
{
	// call onKey Function of active SceneNode
	fboDedist->onKey(key, scancode, action, mods);
}

//--------------------------------------------------------------------------------

void Cs1PerspFboConv::mouseBut(GLFWwindow* window, int button, int action,
		int mods)
{
	fboDedist->mouseBut(window, button, action, mods);
}

//--------------------------------------------------------------------------------

void Cs1PerspFboConv::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
	fboDedist->mouseCursor(window, xpos, ypos);
}

//--------------------------------------------------------------------------------

Cs1PerspFboConv::~Cs1PerspFboConv()
{
	delete[] cam;
}

//--------------------------------------------------------------------------------

void Cs1PerspFboConv::clearFbo()
{
	fboDedist->clearFbo();
}
}
