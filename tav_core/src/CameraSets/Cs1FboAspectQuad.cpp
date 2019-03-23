/*
*	Cs1FboAspectQuad.cpp
*	tav_gl4
*
*	Created by Sven Hahne on 14.08.14.
*	Copyright (c) 2014 Sven Hahne. All rights reserved.
*
*	Standard CameraSetup, die Kamera steht auf (0|0|1) und sieht auf den Ursprung
*	sichtbarer Bereich ist ca. von (-1|+1)|(-1|+1)|(0|-100)
*	je nach Seitenverhältnis. Near ist 1.f und Far 100.0f (sichtbarer Bereich beginnt bei z=0)
*
*	Rendert in einen FBO mit aspect relativem Frustum (korrekte proportionen)
*	FboViews werden als Quadrate gezeichent (kein Warping)
*
*/

#include "pch.h"
#include "Cs1FboAspectQuad.h"

#define STRINGIFY(A) #A

namespace tav
{
Cs1FboAspectQuad::Cs1FboAspectQuad(sceneData* _scd, OSCData* _osc,
		std::vector<fboView*>* _fboViews, GWindowManager* _winMan) :
		CameraSet(1, _scd, _osc), winMan(_winMan)
{
	cam = new GLMCamera*[nrCameras];

	lookAt[0] = glm::vec3(0.f, 0.f, 0.f);
	upVec[0] = glm::vec3(0.f, 1.f, 0.f);

	fboWidth = _scd->fboWidth;
	fboHeight = _scd->fboHeight;
	id = _scd->id;

	float aspect = float(fboWidth) / float(fboHeight);

	cam[0] = new GLMCamera(GLMCamera::FRUSTUM, fboWidth, fboHeight,
			-aspect, aspect, -1.0f, 1.0f,                // left, right, bottom, top
			_scd->camPos.x, _scd->camPos.y, _scd->camPos.z, _scd->camLookAt.x,
			_scd->camLookAt.y, _scd->camLookAt.z,   // lookAt
			_scd->camUpVec.x, _scd->camLookAt.y, _scd->camLookAt.z, 1.f, 100.f);

	// camera zum rendern der fboviews
	stdCam = new GLMCamera(GLMCamera::FRUSTUM,
			_scd->screenWidth, _scd->screenHeight,	// viewport size
			-1.f, 1.f, -1.0f, 1.0f, 				// left, right, bottom, top
			0.f, 0.f, 1.f,                          // camPos
			0.f, 0.f, 0.f,   						// lookAt
			0.f, 1.f, 0.f, 							// upVec
			1.f, 100.f);							// near, far

	quad->setColor(0.f, 0.f, 0.f, 0.f);

	setupCamPar(true);

	// fixed fbo size
	renderFbo = new FBO(_scd->shaderCollector, fboWidth, fboHeight, GL_RGBA8,
			GL_TEXTURE_2D, true, 1, 0, 1, GL_CLAMP_TO_EDGE, false);

	stdTex = shCol->getStdTex();
	initFboShader();

	// create de distortion matrizes
	for (std::vector<fboView*>::iterator it = _fboViews->begin(); it != _fboViews->end(); ++it)
	{
		if ((*it)->srcCamId == id){
			std::cout << "Cs1FboAspectQuad::Cs1FboAspectQuad iterating over fbo views, got FboView with srcCamId " << (*it)->srcCamId << " which is equal to this cam id: " << id << " calculating its matrices" << std::endl;
			fboViews.push_back(*it);
			getPerspTrans(it);
		}
	}
}

//--------------------------------------------------------------------------------

void Cs1FboAspectQuad::initProto(ShaderProto* _proto)
{
	_proto->defineVert(false);
	if (!_proto->enableShdrType[GEOMETRY])
		_proto->defineGeom(false);
	_proto->defineFrag(false);
	_proto->assemble();
}

//--------------------------------------------------------------------------------

void Cs1FboAspectQuad::renderTree(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{
	renderFbo->bind();
	renderFbo->clear();

	iterateNode(_scene, time, dt, ctxNr);

	renderFbo->unbind();
}

//--------------------------------------------------------------------------------

void Cs1FboAspectQuad::render(SceneNode* _scene, double time, double dt, unsigned int ctxNr)
{
	//glScissor(0, 0, fboWidth, fboHeight);

	// call update method of the scene, if blending is enabled
	// this already gets called in the scene blender. the resulting morphscene
	// will have a empty update method
	_scene->updateOscPar(time, dt);
	_scene->update(time, dt);

	if (lightProto.find(_scene->protoName) != lightProto.end())
	{
		lightProto[_scene->protoName]->preRender(_scene, time, dt); // pre rendering e.g. for shadow maps
		lightProto[_scene->protoName]->shader->begin();
		lightProto[_scene->protoName]->sendPar(1, cp, osc, time);   // 1 cameras

		cp.camId = id;
		cp.actFboSize = glm::vec2(float(fboWidth), float(fboHeight));

		_scene->draw(time, dt, &cp, lightProto[_scene->protoName]->shader);
	}
	else
	{
		printf("CameraSet Error: couldn´t get LightProto \n");
	}
}

//--------------------------------------------------------------------------------

void Cs1FboAspectQuad::renderFbos(double time, double dt, unsigned int ctxNr)
{
	//std::cout << "renderFbos: viewport 0, 0, " << scd->screenWidth << ", " <<  scd->screenHeight  << std::endl;

	// set viewport to glfw view
	glViewport(0, 0, scd->screenWidth, scd->screenHeight);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	fboDrawShdr->begin();
	fboDrawShdr->setUniform1i("tex", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderFbo->getColorImg());

	for (std::vector<fboView*>::iterator it = fboViews.begin(); it != fboViews.end(); ++it)
	{
		// check if something was changed via osc: recalc matrixes
		if ((*it)->update)
		{
			//std::cout << "fboView was changed nr: " << it - fboViews.begin() << std::endl;
			getPerspTrans(it);
			(*it)->update = false;
		}

		//std::cout << "draw fbo nr: " << it - fboViews->begin() << std::endl;
		fboDrawShdr->setUniform2fv("texOffs", &(*it)->fTexOffs[0]);
		fboDrawShdr->setUniform2fv("texSize", &(*it)->fTexSize[0]);
		fboDrawShdr->setUniformMatrix4fv("m_pvm", &(*it)->deDist[0][0]);
		quad->draw();
	}



	/*
	 glViewport(0, 0, scd->screenWidth, scd->screenHeight);
	 glDisable(GL_BLEND);

	 stdTex->begin();
	 stdTex->setIdentMatrix4fv("m_pvm");
	 stdTex->setUniform1i("tex", 0);

	 glActiveTexture(GL_TEXTURE0);
	 glBindTexture(GL_TEXTURE_2D, renderFbo->getColorImg() );

	 quad->draw();

	 glEnable(GL_BLEND);

	 //renderFbo->blit(scrWidth, scrHeight);
*/
}

//--------------------------------------------------------------------------------

void Cs1FboAspectQuad::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void Cs1FboAspectQuad::postRender()
{
}

//--------------------------------------------------------------------------------

void Cs1FboAspectQuad::initFboShader()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
		layout (location=0) in vec4 position;\n
		layout (location=1) in vec3 normal;\n
		layout (location=2) in vec2 texCoord;\n
		layout (location=3) in vec4 color;\n

		uniform mat4 m_pvm;\n
		uniform vec2 texOffs;\n
		uniform vec2 texSize;\n

		out to_fs {
			vec2 texCoord;
		} vertex_out;

		void main(void) {
			vertex_out.texCoord = texCoord;
//			vertex_out.texCoord = texCoord * texSize + texOffs;
			gl_Position = m_pvm * position;
		});
	vert = "// Cs1FboAspectQuad record shader\n" + shdr_Header + vert;

	std::string frag = STRINGIFY(
		layout(location = 0) out vec4 color;

		in to_fs {
			vec2 texCoord;
		} vertex_in;

		uniform sampler2D tex;\n

		void main() {
			color = texture(tex, vertex_in.texCoord);
			//color = vec4(1.0, 0.0, 0.0, 1.0);
		});

	frag = "// Cs1FboAspectQuad record shader\n" + shdr_Header + frag;

	fboDrawShdr = shCol->addCheckShaderText("Cs1FboAspectQuad_drawShdr",
			vert.c_str(), frag.c_str());
}

//---------------------------------------------------------

// get perspective transformation in pixels
// upper left corner is origin
void Cs1FboAspectQuad::getPerspTrans(std::vector<fboView*>::iterator it)
{
	std::vector<glm::ivec2> corners;
	corners.push_back((*it)->lowLeft);
	corners.push_back((*it)->lowRight);
	corners.push_back((*it)->upRight);
	corners.push_back((*it)->upLeft);

	// berechne groesse
	glm::vec2 size = glm::vec2(float((*it)->upRight.x - (*it)->lowLeft.x),
			float((*it)->upRight.y - (*it)->lowLeft.y));

	glm::vec2 mid = glm::vec2(size.x * 0.5 + float((*it)->lowLeft.x),
							  size.y * 0.5 + float((*it)->lowLeft.y));

	// in normalisiertem raum
	size.x /= float(scd->screenWidth);
	size.y /= float(scd->screenHeight);

	mid.x = (mid.x / float(scd->screenWidth)) * 2.f - 1.f;
	mid.y = (mid.y / float(scd->screenHeight)) * 2.f - 1.f;

	// berechne skalierung
	(*it)->deDist = glm::translate(glm::vec3(mid.x, mid.y, 0.f)) * glm::scale(glm::vec3(size.x, size.y, 1.f));

	// berechne textur offsets
	(*it)->fTexSize = glm::vec2(float((*it)->texSize.x) / float(fboWidth),
			float((*it)->texSize.y) / float(fboHeight));

	(*it)->fTexOffs = glm::vec2(float((*it)->texOffs.x) / float(fboWidth),
			float((*it)->texOffs.y) / float(fboHeight));
}

//---------------------------------------------------------

void Cs1FboAspectQuad::onKey(int key, int scancode, int action, int mods)
{
	// call onKey Function of active SceneNode
}

//--------------------------------------------------------------------------------

void Cs1FboAspectQuad::mouseBut(GLFWwindow* window, int button, int action,
		int mods)
{
}

//--------------------------------------------------------------------------------

void Cs1FboAspectQuad::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
}

//--------------------------------------------------------------------------------

void Cs1FboAspectQuad::clearFbo()
{
}

//--------------------------------------------------------------------------------

Cs1FboAspectQuad::~Cs1FboAspectQuad()
{
	delete[] cam;
}
}
