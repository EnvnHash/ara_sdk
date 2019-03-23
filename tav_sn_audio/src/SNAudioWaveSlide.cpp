//
// SNAudioWaveSlide.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Funktioniert nur mit blending = true
//

#include "SNAudioWaveSlide.h"

#define STRINGIFY(A) #A

namespace tav
{

SNAudioWaveSlide::SNAudioWaveSlide(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "LitSphere"), texGridSize(128)
{
	pa = (PAudio*) scd->pa;
	osc = (OSCData*) scd->osc;
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	getChanCols(&chanCols);

	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];

	osc->addPar("sliderScale", 0.f, 2.f, 0.01f, 0.22f, OSCData::LIN);
	osc->addPar("yOffs", 0.f, 1.f, 0.01f, 0.11f, OSCData::LIN);
	osc->addPar("zOffs", 0.f, 1.f, 0.01f, 0.2f, OSCData::LIN);
	osc->addPar("slidScaleY", 0.f, 2.f, 0.001f, 1.07f, OSCData::LIN);
	osc->addPar("slidScaleZ", 0.f, 2.f, 0.001f, 0.37f, OSCData::LIN);
	osc->addPar("audioMed", 0.f, 70.f, 0.001f, 2.31f, OSCData::LIN);
	osc->addPar("audioAmp", 0.f, 15.f, 0.001f, 3.0f, OSCData::LIN);

	quadAr = new QuadArray(110, 110, -1.f, -1.f, 1.0f, 1.0f, 1.f, 1.f, 1.f,
			1.f);
	//quadAr->rotate(float(M_PI) * 0.5f, 1.f, 0.f, 0.f);

	quad = scd->stdQuad;

	modelMat = glm::mat4(1.f);
	normalMat = glm::mat3(glm::transpose(glm::inverse(modelMat)));

	posTex = new FBO(shCol, texGridSize, texGridSize, GL_RGB32F, GL_TEXTURE_2D,
			false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	normTex = new FBO(shCol, texGridSize, texGridSize, GL_RGB32F, GL_TEXTURE_2D,
			false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	pllSmooth = new PingPongFbo(shCol, pa->frames_per_buffer, 1, GL_RGBA32F,
			GL_TEXTURE_1D, false, 1, 1, 1, GL_REPEAT, false);

	stdTex2D = shCol->getStdTex();

	initPreCalcShdr();
	initTex1DShdr();
}

//--------------------------------------------------------------

void SNAudioWaveSlide::draw(double time, double dt, camPar* cp,
		Shaders* _shader, TFO* _tfo)
{
	if (!inited)
	{
		initWaveShdr(_tfo);
		inited = true;
	}

	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);
		_tfo->end(); // tfo muss angehalten werden, damit der shader gewechselt werden kann
		waveShdr->begin();
		_tfo->begin(GL_TRIANGLES);
		_tfo->enableDepthTest();
	}
	else
	{
		waveShdr->begin();
	}

	glEnable (GL_CULL_FACE);
	glEnable (GL_DEPTH_TEST);
	sendStdShaderInit(waveShdr);
	useTextureUnitInd(0, tex0->getId(), waveShdr, _tfo);

	modelMat = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -1.f));
	//modelMat = glm::rotate(modelMat, float(time * 0.5), glm::vec3(0.f, 1.f, 0.f));
	normalMat = glm::mat3(glm::transpose(glm::inverse(modelMat)));

	waveShdr->setUniformMatrix4fv("modelMatrix", &modelMat[0][0]);
	waveShdr->setUniformMatrix3fv("normalMatrix", &normalMat[0][0]);
	waveShdr->setUniformMatrix4fv("viewMatrix", cp->multicam_view_matrix);
	waveShdr->setUniformMatrix4fv("projectionMatrix",
			cp->multicam_projection_matrix);
	waveShdr->setUniform1i("posTex", 0);
	waveShdr->setUniform1i("normTex", 1);
	waveShdr->setUniform1f("time", float(time));

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, posTex->getColorImg());

	glActiveTexture (GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normTex->getColorImg());

	/*
	 stdTex->begin();
	 stdTex->setIdentMatrix4fv("m_pvm");

	 glActiveTexture(GL_TEXTURE0);
	 glBindTexture(GL_TEXTURE_1D, pllSmooth->getColorImg());
	 */

	quadAr->draw(_tfo);

	if (_tfo)
	{
		_tfo->end(); // tfo muss angehalten werden, damit der shader gewechselt werden kann
		_shader->begin();
		_tfo->begin(GL_TRIANGLES);
	}
}

//--------------------------------------------------------------

void SNAudioWaveSlide::update(double time, double dt)
{
	//scd->audioTex->mergeTo2D( pa->waveData.blockCounter);
	scd->audioTex->update(pa->getPll(), pa->waveData.blockCounter);
	//        scd->audioTex->update(pa->getActFft(), pa->waveData.blockCounter);

	pllSmooth->dst->bind();
	pllSmooth->dst->clear();

	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);
	stdTex->setUniform1i("lastTex", 1);
	stdTex->setUniform1f("med", osc->getPar("audioMed"));

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D, scd->audioTex->getTex(0));

	glActiveTexture (GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_1D, pllSmooth->getSrcTexId());

	quad->draw();

	pllSmooth->dst->unbind();
	pllSmooth->swap();

	//  std::cout << pllSmooth->getColorImg() << std::endl;

	// - pre calculate positions from audioTexture ----

	posTex->bind();
	posTex->clear();

	posTexShdr->begin();
	posTexShdr->setUniform1i("audioTex", 0);
	posTexShdr->setUniform1f("sliderScale", osc->getPar("sliderScale"));
	posTexShdr->setUniform1f("yOffs", osc->getPar("yOffs") + time * 0.1f);
	posTexShdr->setUniform1f("zOffs", osc->getPar("zOffs"));
	posTexShdr->setUniform1f("slidScaleY", osc->getPar("slidScaleY"));
	posTexShdr->setUniform1f("slidScaleZ", osc->getPar("slidScaleZ"));
	posTexShdr->setUniform1f("amp", osc->getPar("audioAmp"));

	glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_1D, scd->audioTex->getTex(0));
	glBindTexture(GL_TEXTURE_1D, pllSmooth->getSrcTexId());

	quad->draw();
	posTex->unbind();

	// - pre calculate normals from precalculated positions -

	normTex->bind();
	normTex->clear();

	normTexShdr->begin();
	normTexShdr->setUniform1i("posTex", 0);
	normTexShdr->setUniform1i("texGridSize", texGridSize);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, posTex->getColorImg());

	quad->draw();
	normTex->unbind();
}

//--------------------------------------------------------------

void SNAudioWaveSlide::initPreCalcShdr()
{
	//- Position Shader ---

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=1) in vec3 normal; layout (location=2) in vec2 texCoord; layout (location=3) in vec4 color; out vec4 pos; out vec2 toTexPos; void main(void) { pos = position; toTexPos = position.xy * 0.5 + 0.5; gl_Position = position; });
	vert = "// AudioWaveSlide pos tex vertex shader\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 pos_tex; in vec4 pos; in vec2 toTexPos; uniform sampler1D audioTex; uniform int nrChannels; uniform float sliderScale; uniform float yOffs; uniform float zOffs; uniform float slidScaleY; uniform float slidScaleZ;

					float x; float y; float z; uniform float amp; int i; float slider; float signChange; void main() { slider = toTexPos.y * sliderScale; signChange = sin(toTexPos.x * 6.14);
					//signChange = 1.0;

					x = texture(audioTex, toTexPos.x + slider).r * signChange; y = texture(audioTex, toTexPos.x + yOffs + slider * slidScaleY).r * signChange; z = texture(audioTex, toTexPos.x + zOffs + slider * slidScaleZ).r * signChange;
					// z -= toTexPos.y;

					pos_tex = vec4(vec3(x, y, z) * amp, 1.0); });
	frag = "// AudioWaveSlide pos tex shader\n" + shdr_Header + frag;

	posTexShdr = shCol->addCheckShaderText("AudioWaveSlidePosTex", vert.c_str(),
			frag.c_str());

	//- Normal Shader ---

	vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=1) in vec3 normal; layout (location=2) in vec2 texCoord; layout (location=3) in vec4 color; out vec2 toTexPos; void main(void) { toTexPos = vec2(position) * 0.5 + 0.5; gl_Position = position; });
	vert = "// AudioWaveSlide norm tex vertex shader\n" + shdr_Header + vert;

	frag =
			STRINGIFY(
					layout(location = 0) out vec4 norm_tex; in vec2 toTexPos; uniform sampler2D posTex; uniform int texGridSize;

					vec3 posTop; vec3 posBottom; vec3 posCenter; vec3 posLeft; vec3 posRight;

					vec3 norms[2];

					float gridOffs; void main() { gridOffs = 1.0 / float(texGridSize);

					// read neighbour positions left, right, top, bottom
					posTop = texture(posTex, vec2(toTexPos.x, toTexPos.y + gridOffs) ).xyz; posBottom = texture(posTex, vec2(toTexPos.x, toTexPos.y - gridOffs)).xyz; posCenter = texture(posTex, toTexPos).xyz; posLeft = texture(posTex, vec2(toTexPos.x - gridOffs, toTexPos.y)).xyz; posRight = texture(posTex, vec2(toTexPos.x + gridOffs, toTexPos.y)).xyz;

					norms[0] = normalize(cross((posTop - posCenter), (posLeft - posCenter))); norms[1] = normalize(cross((posBottom - posCenter), (posRight - posCenter)));

					if(sign(norms[0]) != sign(norms[1])) norms[0] *= -1.0;

					norms[0] = normalize((norms[0] + norms[1]) * 0.5); norm_tex = vec4(norms[0], 1.0); });
	frag = "// AudioWaveSlide norm tex shader\n" + shdr_Header + frag;

	normTexShdr = shCol->addCheckShaderText("AudioWaveSlideNormTex",
			vert.c_str(), frag.c_str());
}

//--------------------------------------------------------------

void SNAudioWaveSlide::initTex1DShdr()
{
	//- Position Shader ---

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=1) in vec3 normal; layout (location=2) in vec2 texCoord; layout (location=3) in vec4 color; out vec2 tex_coord; void main(void) { tex_coord = texCoord; gl_Position = position; });
	vert = "// AudioWaveSlide  audio smooth vertex shader\n" + shdr_Header
			+ vert;

	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 color; in vec2 tex_coord; uniform sampler1D tex; uniform sampler1D lastTex; uniform float med; void main() { float last = texture(lastTex, tex_coord.x).r; float act = texture(tex, tex_coord.x).r; color = vec4((act + last * med) / (med + 1.0), 0.0, 0.0, 1.0); });
	frag = "// AudioWaveSlide audio smooth tex shader\n" + shdr_Header + frag;

	stdTex = shCol->addCheckShaderText("AudioWaveSlideAudio1dTex", vert.c_str(),
			frag.c_str());
}

//--------------------------------------------------------------

void SNAudioWaveSlide::initWaveShdr(TFO* _tfo)
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=1) in vec3 normal; layout (location=2) in vec2 texCoord; layout (location=3) in vec4 color;

					uniform sampler2D posTex; uniform sampler2D normTex;

					uniform int useInstancing; uniform int texNr; uniform float time;

					uniform mat4 modelMatrix; uniform mat3 normalMatrix; uniform mat4 viewMatrix; uniform mat4 projectionMatrix;

					out vec4 rec_position; out vec3 rec_normal; out vec4 rec_texCoord; out vec4 rec_color;

					vec2 modTexCoord; void main(void) { modTexCoord = vec2(texCoord.x, texCoord.y);
					//                                         modTexCoord = vec2(texCoord.x, mod(texCoord.y - time * 0.1, 1.0));
					rec_position = modelMatrix * texture(posTex, texCoord); rec_normal = normalize(normalMatrix * texture(normTex, texCoord).xyz); rec_color = color; rec_texCoord = vec4(modTexCoord, float(texNr), 0.0);

					gl_Position = rec_position; });
	vert = "// AudioWaveSlide plane shader\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 color; void main() { color = vec4(1.0); });
	frag = "// AudioWaveSlide plane shader\n" + shdr_Header + frag;

	waveShdr = shCol->addCheckShaderTextNoLink("AudioWaveSlideFinal",
			vert.c_str(), frag.c_str());

	//- Setup TFO ---

	std::vector<std::string> names;
	// copy the standard record attribute names, number depending on hardware
	for (auto i = 0; i < MAX_SEP_REC_BUFS; i++)
		names.push_back(stdRecAttribNames[i]);
	_tfo->setVaryingsToRecord(&names, waveShdr->getProgram());

	waveShdr->link();
}

//--------------------------------------------------------------

SNAudioWaveSlide::~SNAudioWaveSlide()
{
	delete posTex;
	delete normTex;
	delete quadAr;
	delete normTexShdr;
	delete posTexShdr;
	delete waveShdr;
}
}
