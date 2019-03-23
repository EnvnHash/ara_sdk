//
// SNAudioTunnelGPU_Perl.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Funktioniert nur mit blending = true
//

#include "SNAudioTunnelGPU_Perl.h"

#define STRINGIFY(A) #A

namespace tav
{
SNAudioTunnelGPU_Perl::SNAudioTunnelGPU_Perl(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "PerlinTexCoord"), texGridSize(64), nrSegs(
				60)
{
	pa = (PAudio*) scd->pa;
	osc = (OSCData*) scd->osc;
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	getChanCols(&chanCols);

	quadAr = new QuadArray(nrSegs, nrSegs, -1.f, -1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			1.f);
	//quadAr->rotate(float(M_PI) * 0.5f, 1.f, 0.f, 0.f);

	quad = scd->stdQuad;

	modelMat = glm::mat4(1.f);
	normalMat = glm::mat3(glm::transpose(glm::inverse(modelMat)));

	posTex = new FBO(shCol, texGridSize, texGridSize, GL_RGB16F, GL_TEXTURE_2D,
			false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	normTex = new FBO(shCol, texGridSize, texGridSize, GL_RGB16F, GL_TEXTURE_2D,
			false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);

	img = new TextureManager();
	img->loadTexture2D((*scd->dataPath) + "textures/mann.jpg");

	noiseTex = new Noise3DTexGen(shCol, false, 4, 256, 256, 64, 4.f, 4.f, 8.f);

	initPreCalcShdr();
}

//----------------------------------------------------

void SNAudioTunnelGPU_Perl::draw(double time, double dt, camPar* cp,
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
		useTextureUnitInd(0, scd->audioTex->getTex2D(), _shader, _tfo);
		_tfo->addTexture(1, noiseTex->getTex(), GL_TEXTURE_3D, "perlinTex");
		_tfo->addTexture(2, img->getId(), GL_TEXTURE_2D, "fotoTex");

	}
	else
	{
		waveShdr->begin();
	}

	sendStdShaderInit(waveShdr);

	modelMat = glm::rotate(glm::mat4(1.f), float(M_PI) * -0.5f,
			glm::vec3(1.f, 0.f, 0.f));
	normalMat = glm::mat3(glm::transpose(glm::inverse(modelMat)));

	waveShdr->setUniformMatrix4fv("modelMatrix", &modelMat[0][0]);
	waveShdr->setUniformMatrix3fv("normalMatrix", &normalMat[0][0]);
	waveShdr->setUniformMatrix4fv("projectionMatrix",
			cp->multicam_projection_matrix);
	waveShdr->setUniform1f("texGridSize", float(texGridSize));
	waveShdr->setUniform1i("posTex", 0);
	waveShdr->setUniform1i("vertPerChan", nrSegs * nrSegs * 6);
	waveShdr->setUniform1i("normTex", 1);
	waveShdr->setUniform1f("time", float(intTime));
	waveShdr->setUniform4fv("scCol", &chanCols[0][0], scd->nrChannels);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, posTex->getColorImg());

	glActiveTexture (GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normTex->getColorImg());

	quadAr->draw(_tfo);

	if (_tfo)
	{
		_tfo->end(); // tfo muss angehalten werden, damit der shader gewechselt werden kann
		_shader->begin();
		_tfo->begin(GL_TRIANGLES);
	}
}

//----------------------------------------------------

void SNAudioTunnelGPU_Perl::update(double time, double dt)
{
	intTime += dt * osc->speed;

	scd->audioTex->update(pa->getPll(), pa->waveData.blockCounter);
	scd->audioTex->mergeTo2D(pa->waveData.blockCounter);

	// - pre calculate positions from audioTexture ----
	// since linear filtering will produce an edge with weighted pixels
	// the fbo needs to be written with one pixel border

	posTex->bind();
	posTex->clear();

	posTexShdr->begin();
	posTexShdr->setUniform1i("audioTex", 0);
	posTexShdr->setUniform1i("nrChannels", scd->nrChannels);
	posTexShdr->setUniform1f("time", intTime);
	posTexShdr->setIdentMatrix4fv("m_pvm");

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, scd->audioTex->getTex2D());

	quad->draw();

	posTex->unbind();

	// - pre calculate normals from precalculated positions -

	normTex->bind();
	normTex->clear();

	normTexShdr->begin();
	normTexShdr->setUniform1i("posTex", 0);
	normTexShdr->setUniform1f("texGridSize", float(texGridSize));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, posTex->getColorImg());

	quad->draw();

	normTex->unbind();
}

//----------------------------------------------------

void SNAudioTunnelGPU_Perl::initPreCalcShdr()
{
	//- Position Shader ---
	// leave one pixel border

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=1) in vec3 normal; layout (location=2) in vec2 texCoord; layout (location=3) in vec4 color; uniform mat4 m_pvm; out vec4 pos; out vec2 toTexPos;

					void main(void) { pos = position; toTexPos = position.xy * 0.5 + 0.5; gl_Position = m_pvm * position; });
	vert = "// AudioTunnelGPU pos tex vertex shader\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 pos_tex; in vec4 pos; in vec2 toTexPos; uniform sampler2D audioTex; uniform int nrChannels; uniform float time;

					float ampX; float ampY; float amp2; int i; float chanOffs;

					void main() { ampX = 0.0; ampY = 0.0; amp2 = 0.0;

					for (i=0;i<nrChannels;i++) { chanOffs = float(i) / float(nrChannels); ampX += texture(audioTex, vec2(toTexPos.x, chanOffs)).r * 0.5 + 0.5; ampY += texture(audioTex, vec2(toTexPos.y - time * 0.1, chanOffs)).r * 0.5 + 0.5; amp2 += texture(audioTex, vec2(sqrt(toTexPos.x*toTexPos.x + toTexPos.y*toTexPos.y) * 0.5, chanOffs)).r; }

					ampX /= float(nrChannels); ampY /= float(nrChannels); amp2 /= float(nrChannels);

					pos_tex = vec4(pos.x, pos.y, (ampY + ampX), 1.0); });
	frag = "// AudioTunnelGPU pos tex shader\n" + shdr_Header + frag;

	posTexShdr = shCol->addCheckShaderText("audioTunnelGPUPosTex", vert.c_str(),
			frag.c_str());

	//- Normal Shader ---
	// relevant ist immer die mitte des Pixels, deshalb muessen die TexturKoordinate
	// geringfuegig um ein halbes Pixels auf allen Seiten verkleinert werden

	vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=1) in vec3 normal; layout (location=2) in vec2 texCoord; layout (location=3) in vec4 color;

					uniform float texGridSize; out vec2 toTexPos;

					void main(void) { toTexPos = (position.xy * 0.5 + 0.5) * ((texGridSize - 1.0) / texGridSize) + vec2(0.5 / texGridSize); gl_Position = position; });
	vert = "// AudioTunnelGPU norm tex vertex shader\n" + shdr_Header + vert;

	frag =
			STRINGIFY(
					layout(location = 0) out vec4 norm_tex; in vec2 toTexPos; uniform sampler2D posTex; uniform float texGridSize;

					vec3 posTop; vec3 posBottom; vec3 posCenter; vec3 posLeft; vec3 posRight;

					vec3 norms[2];

					float gridOffs;

					void main() { gridOffs = 1.0 / float(texGridSize);

					// read neighbour positions left, right, top, bottom
					posTop = texture(posTex, vec2(toTexPos.x, toTexPos.y + gridOffs) ).xyz; posBottom = texture(posTex, vec2(toTexPos.x, toTexPos.y - gridOffs)).xyz; posCenter = texture(posTex, toTexPos).xyz; posLeft = texture(posTex, vec2(toTexPos.x - gridOffs, toTexPos.y)).xyz; posRight = texture(posTex, vec2(toTexPos.x + gridOffs, toTexPos.y)).xyz;

					norms[0] = normalize(cross((posTop - posCenter), (posLeft - posCenter))); norms[1] = normalize(cross((posBottom - posCenter), (posRight - posCenter)));

					for(int i=0;i<2;i++) norms[i] = norms[i].z > 0.0 ? norms[i] : norms[i] * -1.0;

					norms[0] = normalize((norms[0] + norms[1]) * 0.5); norm_tex = vec4(norms[0], 1.0); });
	frag = "// AudioTunnelGPU norm tex shader\n" + shdr_Header + frag;

	normTexShdr = shCol->addCheckShaderText("audioTunnelGPUNormTex",
			vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNAudioTunnelGPU_Perl::initWaveShdr(TFO* _tfo)
{
	std::string shdr_Header =
			"#version 410 core\n#pragma optimize(on)\n uniform vec4 scCol["
					+ std::to_string(scd->nrChannels) + "];\n";
	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=1) in vec3 normal; layout (location=2) in vec2 texCoord; layout (location=3) in vec4 color;

					uniform sampler2D posTex; uniform sampler2D normTex;

					uniform int useInstancing; uniform int vertPerChan; uniform int texNr; uniform float time; uniform float texGridSize;

					uniform mat4 modelMatrix; uniform mat4 projectionMatrix; uniform mat3 normalMatrix; mat4 normCylMatrix;

					out vec4 rec_position; out vec3 rec_normal; out vec4 rec_texCoord; out vec4 rec_color;

					vec2 modTexCoord; vec4 pos; vec4 norm; vec4 column0; vec4 column1; vec4 column2; vec4 column3;

					float pi = 3.1415926535897932384626433832795; float rad; float angle; int chanInd;

					void main(void) { chanInd = gl_VertexID / vertPerChan; modTexCoord = texCoord * ((texGridSize - 1.0) / texGridSize) + vec2(0.5 / texGridSize);

					pos = texture(posTex, modTexCoord); pos = vec4(pos.xy * ((texGridSize + 1.0) / texGridSize) * 1.0001, // noetig, vermutlich rundungsfehler, kreis wird sonst nicht ganz geschlossen
					pos.z, 1.0); angle = pos.x * pi;

					rad = pos.z + 0.5; pos = vec4(cos(angle) * rad, (pos.y + 1.0) * 16.0, sin(angle) * rad, 1.0); rec_position = modelMatrix * pos;

					norm = texture(normTex, modTexCoord);

					column0 = vec4(cos(angle), 0.0, -sin(angle), 0.0); column1 = vec4(0.0, 1.0, 0.0, 0.0); column2 = vec4(sin(angle), 0.0, cos(angle), 0.0); column3 = vec4(0.0, 0.0, 0.0, 1.0); normCylMatrix = mat4(column0, column1, column2, column3);

					rec_normal = normalize(normalMatrix * (normCylMatrix * norm).xyz);

					rec_color = scCol[chanInd]; rec_texCoord = vec4(vec2(texCoord.x, texCoord.y - time * 0.1), float(texNr), 0.0);

					gl_Position = rec_position; });
	vert = "// AudioTunnelGPU plane shader\n" + shdr_Header + vert;

	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 color; void main() { color = vec4(1.0); });
	frag = "// AudioTunnelGPU plane shader\n" + shdr_Header + frag;

	waveShdr = shCol->addCheckShaderTextNoLink("audioTunnelGPUFinal",
			vert.c_str(), frag.c_str());

	//- Setup TFO ---

	std::vector<std::string> names;
	// copy the standard record attribute names, number depending on hardware
	for (auto i = 0; i < MAX_SEP_REC_BUFS; i++)
		names.push_back(stdRecAttribNames[i]);
	_tfo->setVaryingsToRecord(&names, waveShdr->getProgram());

	waveShdr->link();
}

//----------------------------------------------------

SNAudioTunnelGPU_Perl::~SNAudioTunnelGPU_Perl()
{
	delete posTex;
	delete normTex;
	delete quadAr;
	delete normTexShdr;
	delete posTexShdr;
	delete waveShdr;
}
}
