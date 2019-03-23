//
//  SkyBox.cpp
//  Tav_App
//
//  Created by Sven Hahne on 18/3/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "SkyBox.h"

#define STRINGIFY(A) #A

using namespace tav;

SkyBox::SkyBox(std::string textureFile, unsigned _nrCams)
{
	init(textureFile.c_str(), _nrCams);
}

//---------------------------------------------------------------------

SkyBox::SkyBox(const char* textureFile, unsigned _nrCams)
{
	init(textureFile, _nrCams);
}

//---------------------------------------------------------------------

void SkyBox::init(const char* textureFile, unsigned _nrCams)
{
	vShader =
			STRINGIFY(
					layout (location = 0) in vec4 position; out vec4 pos; void main(void) { pos = position; });
	vShader = "#version 410\n" + vShader;

	gShader =
			STRINGIFY(
					in vec4 pos[]; out vec3 tex_coord; uniform mat4 tc_rot; void main(void) { gl_ViewportIndex = gl_InvocationID; for (int i=0;i<gl_in.length();i++) { tex_coord = normalize( (tc_rot * pos[i]).xyz ); gl_Position = m_pvm[gl_InvocationID] * pos[i]; EmitVertex(); } EndPrimitive(); });

	std::string gHeader = "#version 410\n";
	gHeader += "layout(triangles, invocations=" + std::to_string(_nrCams)
			+ ") in;\n";
	gHeader += "layout(triangle_strip, max_vertices=3) out;\n";
	// gHeader += "uniform mat4 tc_rotate[" +std::to_string(_nrCams)+ "];\n";
	gHeader += "uniform mat4 m_pvm[" + std::to_string(_nrCams) + "];\n";
	gShader = gHeader + gShader;

	fShader =
			STRINGIFY(
					in vec3 tex_coord; layout (location = 0) out vec4 color; uniform samplerCube tex; void main(void) { color = texture(tex, tex_coord); });
	fShader = "#version 410\n" + fShader;

	sbShader = new Shaders(vShader.c_str(), gShader.c_str(), fShader.c_str(),
			false);
	sbShader->link();

	cubeTex = new TextureManager();
	cubeTex->loadTextureCube(textureFile);

	sphere = new Sphere(4.f, 32);

	texUnit = 0;

	pvm = glm::mat4();
}

//---------------------------------------------------------------------

void SkyBox::draw(double time, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	glFrontFace(GL_CW); // vermurkst die darstellung, wenn von aussen drauf gesehen

	glm::mat4 rotMat = glm::rotate(glm::mat4(1.f), float(time) * 0.3f,
			glm::vec3(0.f, 1.f, 0.f));

	sbShader->begin();
	sbShader->setUniformMatrix4fv("tc_rot", (GLfloat*) &rotMat[0][0]);
	sbShader->setUniformMatrix4fv("m_pvm",
			(GLfloat*) &cp->multicam_mvp_mat4[0][0][0], cp->nrCams);
	sbShader->setUniform1ui("samplerCube", texUnit);
	cubeTex->bind(texUnit);

	sphere->draw();

	glFrontFace(GL_CCW); // counter clockwise definition means front, as default
	glDepthMask(GL_TRUE);

	// reconnect shader from previous step
	_shader->begin();
}

//---------------------------------------------------------------------

void SkyBox::setMatr(glm::mat4* _inMatr)
{
	modMatr = _inMatr;
}

//---------------------------------------------------------------------

void SkyBox::remove()
{
	cubeTex->releaseTexture();
	sbShader->remove();
	sphere->remove();
}

//---------------------------------------------------------------------

SkyBox::~SkyBox()
{
//    delete sphere;
//    delete sbShader;
//    delete cubeTex;
}
