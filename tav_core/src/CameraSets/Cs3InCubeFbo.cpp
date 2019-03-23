//
//  Cs3InCubeFbo.cpp
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

//  momentan nur gleiche groessen von fenster
//

#include "pch.h"
#include "Cs3InCubeFbo.h"

#define STRINGIFY(A) #A

namespace tav
{
Cs3InCubeFbo::Cs3InCubeFbo(sceneData* _scd, OSCData* _osc, std::vector<fboView*>* _fboViews,
		GWindowManager* _winMan) :
		CameraSet(3, _scd, _osc), winMan(_winMan)
{
	id = _scd->id;

	overlap = 0.1f;

	fileName = (*scd->dataPath) + "cs3_undist.yml";

	cam = new GLMCamera*[nrCameras];

	lookAt[0] = glm::vec3(-1.f, 0.f, 0.f); // links
	lookAt[1] = glm::vec3(0.f, 0.f, -1.f); // vorne
	lookAt[2] = glm::vec3(1.f, 0.f, 0.f); // rechts

	for (int i = 0; i < nrCameras; i++)
		cam[i] = new GLMCamera(GLMCamera::FRUSTUM, scrWidth / nrCameras,
				scrHeight, -1.f, 1.f,                             // left, right
				-1.f, 1.f,                               // bottom, top
				0.f, 0.f, 0.f,                           // camPos
				lookAt[i].x, lookAt[i].y, lookAt[i].z);  // lookAt

	quad->setColor(0.f, 0.f, 0.f, 1.f);
	setupCamPar();

	quadAr = new QuadArray(40, 40, -1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f));

	renderFbo = new FBO(shCol, scrWidth, scrHeight, GL_RGBA8,
	GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false);

	//texShader = shCol->getStdTex();
	setupRenderFboShader();

	for (int i = 0; i < 4; i++)
	{
		corners.push_back(glm::vec2(0.f, 0.f));
		corners.push_back(glm::vec2(1.f, 0.f));
		corners.push_back(glm::vec2(0.f, 1.f));
		corners.push_back(glm::vec2(1.f, 1.f));
	}

	cornersNames.push_back("BL");
	cornersNames.push_back("BR");
	cornersNames.push_back("TL");
	cornersNames.push_back("TR");

	loadCalib();
}

//--------------------------------------------------------------------------------

Cs3InCubeFbo::~Cs3InCubeFbo()
{
	delete renderFbo;
	delete texShader;
	delete[] cam;
}

//--------------------------------------------------------------------------------

void Cs3InCubeFbo::initProto(ShaderProto* _proto)
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
/*
 void Cs3InCubeFbo::clearScreen()
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
void Cs3InCubeFbo::render(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	// call update method of the scene, if blending is enabled
	// this already gets called in the scene blender. the resulting morphscene
	// will have a empty update method
//        if (ctxNr == 0)
//        {
	_scene->updateOscPar(time, dt);
	_scene->update(time, dt);

	if (lightProto.find(_scene->protoName) != lightProto.end())
	{
		lightProto[_scene->protoName]->preRender(_scene, time, dt);

		renderFbo->bind();
		renderFbo->clearAlpha(osc->feedback);

		lightProto[_scene->protoName]->shader->begin();
		lightProto[_scene->protoName]->sendPar(nrCameras, cp, osc, time);

		// set viewports
		//const float wot = float(scrWidth);
		const float wot = float(scrWidth) / 3.f;
		glViewportIndexedf(0, 0.f, 0.f, wot, scrHeight);
		glViewportIndexedf(1, wot, 0.f, wot, scrHeight);
		glViewportIndexedf(2, wot * 2.f, 0.f, wot, scrHeight);
		cp.camId = id;

		_scene->draw(time, dt, &cp, lightProto[_scene->protoName]->shader);

		lightProto[_scene->protoName]->shader->end();
		renderFbo->unbind();
	}
	else
	{
		printf("CameraSet Error: couldn´t get LightProto \n");
	}

//        }
}

//--------------------------------------------------------------------------------

void Cs3InCubeFbo::renderFbos(double time, double dt, unsigned int ctxNr)
{
	const float wot = float(scrWidth) / 3.f;

	for (int scr = 0; scr < 3; scr++)
	{
		glViewportIndexedf(0, wot * float(scr), 0.f, wot, scrHeight);

		// render the fbo
		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);
		texShader->setUniform1f("slWidth", 0.3333f);
		texShader->setUniform1f("slXOffs", float(scr) * 0.3333f);

		for (int i = 0; i < 4; i++)
			texShader->setUniform2f(cornersNames[i], corners[scr * 4 + i].x,
					corners[scr * 4 + i].y);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, renderFbo->getColorImg());
		quad->draw();
	}
}

//--------------------------------------------------------------------------------

void Cs3InCubeFbo::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs3InCubeFbo::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs3InCubeFbo::setupRenderFboShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color; uniform mat4 m_pvm; uniform float slWidth; uniform float slXOffs; uniform vec2 BL; uniform vec2 BR; uniform vec2 TL; uniform vec2 TR; uniform vec2 renderSize;

					out vec2 tex_coord; out vec4 col;

					void main() {
					// transform from QC object coords to 0...1
					vec2 p = (vec2(position.x, position.y) + 1.0) * 0.5;

					// interpolate bottom edge x coordinate
					vec2 x1 = mix(BL, BR, p.x);

					// interpolate top edge x coordinate
					vec2 x2 = mix(TL, TR, p.x);

					// interpolate y position
					p = mix(x1, x2, p.y);

					// transform from 0...1 to QC screen coords
					p = (p - 0.5) * 2.0;

					col = color; tex_coord = vec2(texCoord.x * slWidth + slXOffs, texCoord.y); gl_Position = m_pvm * vec4(p, 0, 1); });

	vert = "// cs3InCubeFbo, vert\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					uniform sampler2D tex; in vec2 tex_coord; in vec4 col; layout (location = 0) out vec4 color;

					void main() { color = texture(tex, tex_coord) + col; });

	frag = "// cs3InCubeFbo shader, frag\n" + shdr_Header + frag;

	texShader = shCol->addCheckShaderText("cs3InCubeFbo", vert.c_str(),
			frag.c_str());
}

//--------------------------------------------------------------------------------

void Cs3InCubeFbo::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (mods == GLFW_MOD_SHIFT)
		{
			switch (key)
			{
			case GLFW_KEY_S:
				saveCalib();
				printf("save Settings\n");
				break;
			}
		}
	}
}

//--------------------------------------------------------------------------------

void Cs3InCubeFbo::mouseBut(GLFWwindow* window, int button, int action,
		int mods)
{
	//  unsigned int ctxInd = winMan->getInd(window) * 4;

	unsigned int ctxInd = int(mouseX / (float(scrWidth) * 0.33333f));
	std::cout << "ctx: " << ctxInd << std::endl;

	float xPos = ((mouseX / float(scrWidth)) - float(ctxInd) * 0.3333f) * 3.f;
	float yPos = 1.0 - (mouseY / float(scrHeight));

	std::cout << "xPos: " << xPos << " yPos: " << yPos << std::endl;

	// get the corner
	//BL
	if (xPos < 0.5 && yPos < 0.5f)
	{
		std::cout << "BL " << std::endl;
		corners[ctxInd * 4 + 0].x = xPos;
		corners[ctxInd * 4 + 0].y = yPos;
	}

	// BR
	if (xPos > 0.5 && yPos < 0.5f)
	{
		std::cout << "BR " << std::endl;
		corners[ctxInd * 4 + 1].x = xPos;
		corners[ctxInd * 4 + 1].y = yPos;
	}

	// TL
	if (xPos < 0.5 && yPos > 0.5f)
	{
		std::cout << "TL " << std::endl;
		corners[ctxInd * 4 + 2].x = xPos;
		corners[ctxInd * 4 + 2].y = yPos;
	}

	// TR
	if (xPos > 0.5 && yPos > 0.5f)
	{
		std::cout << "TR " << std::endl;
		corners[ctxInd * 4 + 3].x = xPos;
		corners[ctxInd * 4 + 3].y = yPos;
	}
}

//--------------------------------------------------------------------------------

void Cs3InCubeFbo::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
//        printf("Cs3InCubeFbo::mouseCursor %f  %f\n", xpos, ypos);
	mouseX = xpos;
	mouseY = ypos;
}

//--------------------------------------------------------------------------------

void Cs3InCubeFbo::loadCalib()
{
#ifdef HAVE_OPENCV
	printf("loading calibration \n");

	cv::FileStorage fs(fileName, cv::FileStorage::READ);

	if (fs.isOpened())
	{
		for (int j = 0; j < 4; j++)
		{
			for (int i = 0; i < 4; i++)
			{
				fs["corner" + std::to_string(j) + "_" + std::to_string(i) + "x"]
						>> corners[j * 4 + i].x;
				fs["corner" + std::to_string(j) + "_" + std::to_string(i) + "y"]
						>> corners[j * 4 + i].y;
			}
		}
	}
#endif
}

//--------------------------------------------------------------------------------

void Cs3InCubeFbo::saveCalib()
{
#ifdef HAVE_OPENCV
	printf("saving calibration \n");

	cv::FileStorage fs(fileName, cv::FileStorage::WRITE);

	if (fs.isOpened())
	{
		for (int j = 0; j < 4; j++)
		{
			for (int i = 0; i < 4; i++)
			{
				fs
						<< "corner" + std::to_string(j) + "_"
								+ std::to_string(i) + "x"
						<< corners[j * 4 + i].x;
				fs
						<< "corner" + std::to_string(j) + "_"
								+ std::to_string(i) + "y"
						<< corners[j * 4 + i].y;
			}
		}
	}
#endif
}

//--------------------------------------------------------------------------------

void Cs3InCubeFbo::clearFbo()
{
}
}
