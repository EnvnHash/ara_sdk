//
//  KinectPointCloud.cpp
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "KinectPointCloud.h"

#define STRINGIFY(A) #A

namespace tav
{

KinectPointCloud::KinectPointCloud(KinectInput* _kin, ShaderCollector* _shCol,
		int _scrWidth, int _scrHeight) :
		kin(_kin), shCol(_shCol), scrWidth(_scrWidth), scrHeight(_scrHeight)
{
	nis = kin->getNis();

	width = kin->getDepthWidth();
	height = kin->getDepthHeight();

	mtx = new boost::mutex;
	tex = new tav::TextureManager();
	tex->allocate(width, height, GL_RGBA8, GL_BGRA, GL_TEXTURE_2D,
			GL_UNSIGNED_BYTE); // for the userMap

	ppFbo = new PingPongFbo(shCol, width, height, GL_R16F, GL_TEXTURE_2D);
	ppFbo->clear(0.f);

	fboQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f,
			0.f, 0.f, 1.f);

	quad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 1.f);
	quad->rotate(M_PI, 0.f, 0.f, 1.f);
	quad->rotate(M_PI, 0.f, 1.f, 0.f);

	downScaleFact = 5;
	pcQuadAr = new QuadArray(width / downScaleFact, height / downScaleFact,
			-1.f, -1.f, 2.f, 2.f, 0.f, 0.f, 0.f, 1.f);
	pcQuadAr->rotate(M_PI, 0.f, 0.f, 1.f);
	pcQuadAr->rotate(M_PI, 0.f, 1.f, 0.f);

	//-------------------------------------------------------

	std::string header = "#version 410 core\n";

	vertShader = header+STRINGIFY(
	layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 2 ) in vec2 texCoord;
	layout( location = 3 ) in vec4 color;
	uniform mat4 m_pvm;
	out vec2 tex_coord;
	out vec4 col;

	void main() {
		col = color;
		tex_coord = texCoord;
		gl_Position = m_pvm * position;
	});

	fragShader = header+STRINGIFY(
	uniform sampler2D tex;
	uniform sampler2D oldTex;
	uniform sampler2D userMask;
	uniform float thres;
	uniform float addFact;

	in vec2 tex_coord;
	in vec4 col;

	layout (location = 0) out vec4 color;

	void main() {
		vec4 usr = texture(userMask, tex_coord);
		vec4 tCol = texture(tex, tex_coord) * usr;
		vec4 oldTCol = texture(oldTex, tex_coord);
		color = (oldTCol * addFact + tCol * (1.0 - addFact));
	});

	shader = new Shaders(vertShader.c_str(), fragShader.c_str(), false);
	shader->link();

	//-------------------------------------------------------

	vertToSil = header+STRINGIFY(
	layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 2 ) in vec2 texCoord;
	layout( location = 3 ) in vec4 color;

	out VS_GS_VERTEX {
		vec2 tex_coord; vec4 col;
	} vertex_out;

	void main() {
		vertex_out.tex_coord = texCoord;
		vertex_out.col = color;
		gl_Position = vec4(position.x, position.y, position.z, position.w);
	});

	geoToSil = header+STRINGIFY(
	layout(triangles, invocations = 1) in;
	layout(triangle_strip, max_vertices = 3) out;

	in VS_GS_VERTEX {
		vec2 tex_coord;
		vec4 col;
	} vertex_in[];

	out GS_FS_VERTEX {
		vec2 tex_coord;
		vec4 col;
	} vertex_out;

	uniform sampler2D tex;
	uniform mat4 m_pvm;
	uniform float offsZ;
	float depth1;
	float depth2;
	float depth3;
	float maxDist = 0.1;
	float scaleDepth = 1.0;

	void main(void) {
		depth1 = texture(tex, vertex_in[0].tex_coord).r;
		depth2 = texture(tex, vertex_in[1].tex_coord).r;
		depth3 = texture(tex, vertex_in[2].tex_coord).r;

		if (depth1 >= 0.05 && abs(depth1 - depth2) < maxDist && abs(depth3 - depth2) < maxDist && abs(depth3 - depth1) < maxDist)
		{
			gl_Position = m_pvm * vec4(gl_in[0].gl_Position.x, gl_in[0].gl_Position.y, gl_in[0].gl_Position.z -1.0 + depth1 * scaleDepth, gl_in[0].gl_Position.w ); EmitVertex();
			gl_Position = m_pvm * vec4(gl_in[1].gl_Position.x, gl_in[1].gl_Position.y, gl_in[1].gl_Position.z -1.0 +depth2 * scaleDepth, gl_in[1].gl_Position.w ); EmitVertex();
			gl_Position = m_pvm * vec4(gl_in[2].gl_Position.x, gl_in[2].gl_Position.y, gl_in[2].gl_Position.z -1.0 +depth3 * scaleDepth, gl_in[2].gl_Position.w ); EmitVertex();
			EndPrimitive();
		}
	});

	fragToSil = header+STRINGIFY(
		in GS_FS_VERTEX { vec2 tex_coord; vec4 col; } vertex_in;
		layout (location = 0) out vec4 color;
		void main() {
			color = vec4(1.0, 1.0, 1.0, 1.0);
		});

	//-------------------------------------------------------

	toSil_shader = new Shaders(vertToSil.c_str(), geoToSil.c_str(),
			fragToSil.c_str());
	toSil_shader->link();

	draw_shader = shCol->getStdTex();

	nis->setUpdateImg(true);
	nis->setUpdateSkel(true);

	glm::vec3 lookAt = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 upVec = glm::vec3(0.f, 1.f, 0.f);
	cam = new GLMCamera(GLMCamera::FRUSTUM, scrWidth, scrHeight, -1.0f, 1.0f,
			-1.0f, 1.0f,                // left, right, bottom, top
			0.f, 0.f, 1.f,                           // camPos
			lookAt.x, lookAt.y, lookAt.z,   // lookAt
			upVec.x, upVec.y, upVec.z, 1.f, 100.f);
}

//------------------------------------------

KinectPointCloud::~KinectPointCloud()
{
	shader->remove();
	draw_shader->remove();
	toSil_shader->remove();

	delete cam;
	delete mtx;
	delete tex;
	delete quad;
	delete ppFbo;
	delete fboQuad;
	delete pcQuadAr;
}

//------------------------------------------

void KinectPointCloud::proc(bool hasNewImg, Shaders* _shader, TFO* _tfo)
{
	if (hasNewImg)
	{
		if (_tfo)
		{
			_tfo->end();
			glDisable(GL_RASTERIZER_DISCARD);
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		shader->begin();
		shader->setIdentMatrix4fv("m_pvm");
		shader->setUniform1i("tex", 0);
		shader->setUniform1i("oldTex", 1);
		shader->setUniform1i("userMask", 2);
		shader->setUniform1f("thres", thres);
		shader->setUniform1f("addFact", 0.0f);

		ppFbo->dst->bind();

		// upload user map from NiTE
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, tex->getId());
		glTexSubImage2D(GL_TEXTURE_2D,             // target
				0,                          // First mipmap level
				0, 0,                       // x and y offset
				width,              // width and height
				height,
				GL_BGRA,
				GL_UNSIGNED_BYTE, nis->getResImg());

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, ppFbo->getSrcTexId());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(true));

		fboQuad->draw();

		ppFbo->dst->unbind();
		ppFbo->swap();

		if (_tfo)
		{
			glEnable(GL_RASTERIZER_DISCARD);
			_shader->begin(); // extrem wichtig, sonst keine Bindepunkte für TFO!!!
			_tfo->begin(_tfo->getLastMode());
		}
	}
}

//------------------------------------------

void KinectPointCloud::draw(Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->end();
		glDisable(GL_RASTERIZER_DISCARD);
	}

	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glPatchParameteri(GL_PATCH_VERTICES, 3);

	// draw result
	toSil_shader->begin();
	cam->sendMVP(toSil_shader->getProgram(), "m_pvm");
	toSil_shader->setUniform1i("tex", 0);
	toSil_shader->setUniform1f("offsZ", offZ);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ppFbo->getSrcTexId());

	pcQuadAr->draw();

	toSil_shader->end();

	if (_tfo)
	{
		glEnable(GL_RASTERIZER_DISCARD);
		_shader->begin();  // extrem wichtig, sonst keine Bindepunkte für TFO!!!
		_tfo->begin(_tfo->getLastMode());
	}
}

//------------------------------------------

GLint KinectPointCloud::getTexId()
{
	return tex->getId();
}
}
