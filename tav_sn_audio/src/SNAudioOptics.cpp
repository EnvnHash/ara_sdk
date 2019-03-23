//
// SNAudioOptics.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#define STRINGIFY(A) #A

#include "SNAudioOptics.h"

using namespace glm;

namespace tav
{
SNAudioOptics::SNAudioOptics(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "LitSphere"), nrInst(30), timeScale(0.2), nrSegments(
				100)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	pa = (PAudio*) scd->pa;
	osc = (OSCData*) scd->osc;

	// setup chanCols
	getChanCols(&chanCols);

	shCol = shCol;
	stdTex2D = shCol->getStdTex();

	initShader();
	initTex1DShdr();

	addPar("offsetAmp", &offsetAmp);
	addPar("audioAmp", &audioAmp);
	addPar("audioMed", &audioMed);
	addPar("baseScale", &baseScale);
	addPar("alpha", &alpha);

	disk = new Disk(2.f, 2.f, nrSegments, nullptr, 1, 1.f, 1.f, 1.f, 1.f);
	quad = scd->stdQuad;

	pllSmooth = new PingPongFbo(shCol, pa->frames_per_buffer, 1, GL_RGBA32F,
			GL_TEXTURE_1D, false, 1, 1, GL_REPEAT);

	ikinLogo = new PropoImage(*_scd->dataPath + "textures/ikin_logo.png",
			_scd->screenWidth, _scd->screenHeight, 0.32f,
			PropoImage::UPPER_RIGHT, 0.03f);
}

//----------------------------------------------------

void SNAudioOptics::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);
	}

	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
	glBlendFunc(GL_ONE, GL_ONE);

	if (std::floor(std::fmod(time * timeScale * float(nrInst), 2.f)) == 0)
		glClearColor(1.f, 1.f, 1.f, 1.f);
	else
		glClearColor(0.f, 0.f, 0.f, 1.f);

	glClear (GL_COLOR_BUFFER_BIT);
	glBlendEquation (GL_FUNC_SUBTRACT);

	glm::mat4 m_pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4
			* _modelMat;

	stdCol->begin();
	stdCol->setUniformMatrix4fv("m_pvm", &m_pvm[0][0]);
	stdCol->setUniform1f("nrInst", float(nrInst));
	stdCol->setUniform1f("time", float(-time * timeScale));
	stdCol->setUniform1f("nrSegments", float(nrSegments));
	stdCol->setUniform1f("amp", audioAmp);
	stdCol->setUniform1f("offsetAmp", offsetAmp);
	stdCol->setUniform1f("baseScale", baseScale);
	stdCol->setUniform1i("audioTex", 0);
	stdCol->setUniform1f("alpha", alpha * osc->alpha);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D, pllSmooth->getSrcTexId());

	disk->drawInstanced(GL_TRIANGLE_FAN, nrInst, nullptr);

	glBlendFunc(GL_ONE, GL_ONE);
}

//----------------------------------------------------

void SNAudioOptics::update(double time, double dt)
{
	scd->audioTex->update(pa->getPll(), pa->waveData.blockCounter);

	pllSmooth->dst->bind();
	pllSmooth->dst->clear();

	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);
	stdTex->setUniform1i("lastTex", 1);
	stdTex->setUniform1f("med", audioMed);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D, scd->audioTex->getTex(0));

	glActiveTexture (GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_1D, pllSmooth->getSrcTexId());

	quad->draw();

	pllSmooth->dst->unbind();
	pllSmooth->swap();
}

//----------------------------------------------------

void SNAudioOptics::initTex1DShdr()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=1) in vec3 normal; layout (location=2) in vec2 texCoord; layout (location=3) in vec4 color; out vec2 tex_coord; void main(void) { tex_coord = texCoord; gl_Position = position; });
	vert = "// SNAudioOptics  audio smooth vertex shader\n" + shdr_Header
			+ vert;

	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 color; in vec2 tex_coord; uniform sampler1D tex; uniform sampler1D lastTex; uniform float med; void main() { float last = texture(lastTex, tex_coord.x).r; float act = texture(tex, tex_coord.x).r; color = vec4(act * med + last * (1.0 - med), 0.0, 0.0, 1.0); });
	frag = "// SNAudioOptics audio smooth tex shader\n" + shdr_Header + frag;

	stdTex = shCol->addCheckShaderText("AudioOpticsAudio1dTex", vert.c_str(),
			frag.c_str());
}

//----------------------------------------------------

void SNAudioOptics::initShader()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 3 ) in vec4 color;

					uniform mat4 m_pvm; uniform float nrInst; uniform float nrSegments; uniform float amp; uniform float offsetAmp; uniform float baseScale; uniform float time; uniform sampler1D audioTex;

					float pll; float scale; float instOffs; vec2 modPos; vec2 offset; float pi = 3.1415926535897932384626433832795;

					void main() {

					instOffs = gl_InstanceID / nrInst;

					pll = baseScale + texture(audioTex, gl_VertexID / nrSegments + instOffs + sin(time * 2.0) ).r * amp;

					float alpha = ((gl_VertexID -1) / nrSegments) * 2.0 * pi; modPos = vec2(cos(alpha) * pll, sin(alpha) * pll);

					scale = mod( instOffs + time, 1.0 ); scale = (scale * scale) * 1.0;

					offset = vec2(texture(audioTex, instOffs).r , texture(audioTex, instOffs + 0.1).r) * offsetAmp;

					gl_Position = m_pvm * vec4(modPos * scale + offset * scale, 0.0, 1.0); });
	vert = "// SNAudioOptics vertex shader\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					layout (location = 0) out vec4 color; uniform float alpha; void main() { color = vec4(alpha); });
	frag = "// SNAudioOptics Shader\n" + shdr_Header + frag;

	stdCol = shCol->addCheckShaderText("SNAudioOptics", vert.c_str(),
			frag.c_str());
}

//----------------------------------------------------

SNAudioOptics::~SNAudioOptics()
{
	delete disk;
}
}
