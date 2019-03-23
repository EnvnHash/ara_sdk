//
// SNAudioTunnelGPU.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Funktioniert nur mit blending = true
//

#include "SNAudioTunnelGPU.h"

#define STRINGIFY(A) #A

namespace tav
{
SNAudioTunnelGPU::SNAudioTunnelGPU(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "LitSphere"), texGridSize(64), nrSegs(60)
{
	pa = (PAudio*) scd->pa;
	osc = (OSCData*) scd->osc;
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	getChanCols(&chanCols);

	quadAr = new QuadArray(nrSegs, nrSegs, -1.f, -1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			1.f);
	//quadAr->rotate(float(M_PI) * 0.5f, 1.f, 0.f, 0.f);

	quad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 1.f);

	intModelMat = glm::mat4(1.f);
	normalMat = glm::mat3(glm::transpose(glm::inverse(_modelMat)));

	posTex = new FBO(shCol, texGridSize, texGridSize, GL_RGB16F, GL_TEXTURE_2D,
			false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	normTex = new FBO(shCol, texGridSize, texGridSize, GL_RGB16F, GL_TEXTURE_2D,
			false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);

	litsphereTex = new TextureManager();
	litsphereTex->loadTexture2D(
			*scd->dataPath + "/textures/litspheres/Unknown-28.jpeg");
	litsphereTex->setWraping(GL_REPEAT);

	bumpMap = new TextureManager();
	bumpMap->loadTexture2D(
			*scd->dataPath + "/textures/bump_maps/Unknown-2.jpeg");
	bumpMap->setWraping(GL_REPEAT);

	cubeTex = new TextureManager();
	cubeTex->loadTextureCube(
			((*scd->dataPath) + "textures/skyboxsun5deg2.png").c_str());

	addPar("aux1", &aux1);
	addPar("aux2", &aux2);
	addPar("alpha", &alpha);
	addPar("reflAmt", &reflAmt);
	addPar("brightAdj", &brightAdj);
	addPar("heightScale", &heightScale);

	initPreCalcShdr();
	initRenderShdr();
}

//----------------------------------------------------

void SNAudioTunnelGPU::draw(double time, double dt, camPar* cp,
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

		sendStdShaderInit(waveShdr);

		intModelMat = glm::scale(glm::mat4(1.f), glm::vec3(2.5f, 2.5f, 1.f));
		intModelMat = glm::rotate(intModelMat, float(M_PI) * -0.5f,
				glm::vec3(1.f, 0.f, 0.f));
		intModelMat = glm::rotate(intModelMat, float(time * 0.2),
				glm::vec3(0.f, 1.f, 0.f));
		normalMat = glm::mat3(glm::transpose(glm::inverse(intModelMat)));

		waveShdr->setUniformMatrix4fv("modelMatrix", &intModelMat[0][0]);
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

	}
	else
	{
		glm::mat4 modelMat = glm::rotate(_modelMat, aux2 * float(M_PI) * 2.f,
				glm::vec3(0.f, 0.f, 1.f));

		glm::mat4 m_vm = cp->view_matrix_mat4 * modelMat;
		glm::mat4 m_pvm = cp->projection_matrix_mat4 * m_vm;
		glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(modelMat)));

		glEnable (GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glClear (GL_DEPTH_BUFFER_BIT);

		renderShdr->begin();

		renderShdr->setUniformMatrix4fv("m_vm", &m_vm[0][0]);
		renderShdr->setUniformMatrix4fv("m_pvm", &m_pvm[0][0]);
		renderShdr->setUniformMatrix3fv("normalMatrix", &normalMat[0][0]);
		renderShdr->setUniform1f("texGridSize", float(texGridSize));
		renderShdr->setUniform1i("posTex", 0);
		renderShdr->setUniform1i("normTex", 1);
		renderShdr->setUniform1i("litSphereTex", 2);
		renderShdr->setUniform1i("cubeMap", 3);

		renderShdr->setUniform1i("vertPerChan", nrSegs * nrSegs * 6);
		renderShdr->setUniform1f("time", float(intTime));
		renderShdr->setUniform4fv("scCol", &chanCols[0][0], scd->nrChannels);
		renderShdr->setUniform1f("alpha", osc->alpha * alpha);
		renderShdr->setUniform1f("reflAmt", reflAmt);
		renderShdr->setUniform1f("brightAdj", brightAdj);
		renderShdr->setUniform1f("heightScale", heightScale + aux1 * 15.f);

		glActiveTexture (GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, posTex->getColorImg());

		glActiveTexture (GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normTex->getColorImg());

		litsphereTex->bind(2);
		cubeTex->bind(3);

		quadAr->draw();
	}

	if (_tfo)
	{
		_tfo->end(); // tfo muss angehalten werden, damit der shader gewechselt werden kann
		_shader->begin();
		_tfo->begin(GL_TRIANGLES);
	}
}

//----------------------------------------------------

void SNAudioTunnelGPU::update(double time, double dt)
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

void SNAudioTunnelGPU::initPreCalcShdr()
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

					for (i=0;i<nrChannels;i++) { chanOffs = float(i) / float(nrChannels); ampX += texture(audioTex, vec2(toTexPos.x, chanOffs)).r; ampY += texture(audioTex, vec2(toTexPos.y - time * 0.1, chanOffs)).r; amp2 += texture(audioTex, vec2(sqrt(toTexPos.x*toTexPos.x + toTexPos.y*toTexPos.y) * 0.5, chanOffs)).r; }

					ampX /= float(nrChannels); ampY /= float(nrChannels); amp2 /= float(nrChannels);

					pos_tex = vec4(pos.x, pos.y, (ampY + ampX), // -1 to 1
					1.0); });
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

void SNAudioTunnelGPU::initWaveShdr(TFO* _tfo)
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

					pos = texture(posTex, modTexCoord); pos = vec4(pos.xy * ((texGridSize + 1.0) / texGridSize) * 1.0002, // noetig, vermutlich rundungsfehler, kreis wird sonst nicht ganz geschlossen
					pos.z, 1.0); angle = pos.x * pi;

					rad = pos.z + 0.5; rad *= 1.2;

					pos = vec4(cos(angle) * rad, (pos.y + 1.0) * 16.0, sin(angle) * rad, 1.0); rec_position = modelMatrix * pos;

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

void SNAudioTunnelGPU::initRenderShdr()
{
	//std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string shdr_Header =
			"#version 410 core\n#pragma optimize(on)\n uniform vec4 scCol["
					+ std::to_string(scd->nrChannels) + "];\n";

	std::string stdVert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position;\n layout( location = 1 ) in vec4 normal;\n layout( location = 2 ) in vec2 texCoord;\n layout( location = 3 ) in vec4 color;\n

					uniform sampler2D posTex;\n uniform sampler2D normTex;\n

					uniform int useInstancing;\n uniform int vertPerChan;\n uniform int texNr;\n uniform float time;\n uniform float texGridSize;\n uniform float heightScale;

					mat4 normCylMatrix;\n

					uniform mat4 m_vm;\n uniform mat4 m_pvm;\n uniform mat3 normalMatrix;\n

					vec2 modTexCoord;\n vec4 pos;\n vec4 norm;\n vec4 column0;\n vec4 column1;\n vec4 column2;\n vec4 column3;\n

					float pi = 3.1415926535897932384626433832795;\n float rad;\n float angle;\n int chanInd;\n

					out TO_FS {\n vec4 tex_coord;\n vec3 eye_pos;\n vec3 normal;\n vec4 col; } vertex_out;\n

					void main()\n {\n chanInd = gl_VertexID / vertPerChan;\n modTexCoord = texCoord * ((texGridSize - 1.0) / texGridSize) + vec2(0.5 / texGridSize);\n

					pos = texture(posTex, modTexCoord);\n pos = vec4(pos.xy * ((texGridSize + 1.0) / texGridSize) * 1.0002, // noetig, vermutlich rundungsfehler, kreis wird sonst nicht ganz geschlossen
					pos.z, 1.0);\n angle = pos.x * pi;\n rad = pos.z * heightScale + 1.0;\n// ops.z -1 to 1

					pos = vec4(sin(angle) * rad, cos(angle) * rad, (pos.y - 0.5) * 16.0, 1.0);\n

					norm = texture(normTex, modTexCoord);\n

					column0 = vec4(cos(angle), 0.0, -sin(angle), 0.0);\n column1 = vec4(0.0, 1.0, 0.0, 0.0);\n column2 = vec4(sin(angle), 0.0, cos(angle), 0.0);\n column3 = vec4(0.0, 0.0, 0.0, 1.0);\n normCylMatrix = mat4(column0, column1, column2, column3);\n

//		vertex_out.normal = normalize(normalMatrix * norm.xyz);\n
					vertex_out.normal = normalize(normalMatrix * (normCylMatrix * norm).xyz);\n

					vertex_out.col = scCol[chanInd]; vertex_out.tex_coord = vec4(vec2(texCoord.x, texCoord.y - time * 0.1),\n float(texNr), 0.0);\n vertex_out.eye_pos = normalize( vec3( m_vm * pos ) );\n

					gl_Position = m_pvm * pos;\n });

	stdVert = "// SNAudioTunnelGPU vertex shader\n" + shdr_Header + stdVert;

	std::string frag =
			STRINGIFY(
					uniform sampler2D litSphereTex; uniform samplerCube cubeMap;

					uniform mat3 m_normal;

					in TO_FS { vec4 tex_coord; vec3 eye_pos; vec3 normal; vec4 col; } vertex_in;

					vec4 litColor; const float Eta = 0.15; // Water

					uniform float alpha; uniform float reflAmt; uniform float brightAdj;

					layout (location = 0) out vec4 color;

					void main() {
					// litsphere
					vec3 reflection = reflect( vertex_in.eye_pos, vertex_in.normal ); vec3 refraction = refract( vertex_in.eye_pos, vertex_in.normal, Eta );

					float m = 2.0 * sqrt( pow( reflection.x, 2.0 ) + pow( reflection.y, 2.0 ) + pow( reflection.z + 1.0, 2.0 ) ); vec2 vN = reflection.xy / m + 0.5;

					litColor = texture(litSphereTex, vN); float litBright = litColor.r * litColor.g * litColor.b;

					vec4 reflectionCol = texture( cubeMap, reflection ); vec4 refractionCol = texture( cubeMap, refraction );

					float fresnel = Eta + (1.0 - Eta) * pow(max(0.0, 1.0 - dot(-vertex_in.eye_pos, vertex_in.normal)), 5.0); vec4 reflCol = reflAmt * mix(reflectionCol, refractionCol, min(max(fresnel, 0.0), 1.0));

					color = vertex_in.col * litBright * brightAdj + reflCol;

					color.a = alpha; });

	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	frag = "// SNAudioTunnelGPU fragment shader\n" + shdr_Header + frag;

	renderShdr = shCol->addCheckShaderText("SNAudioTunnelGPU_rndr",
			stdVert.c_str(), frag.c_str());
}

//----------------------------------------------------

SNAudioTunnelGPU::~SNAudioTunnelGPU()
{
	delete posTex;
	delete normTex;
	delete quadAr;
	delete normTexShdr;
	delete posTexShdr;
	delete waveShdr;
}
}
