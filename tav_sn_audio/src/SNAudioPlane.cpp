//
// SNAudioPlane.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Funktioniert nur mit blending = true
//

#include "SNAudioPlane.h"

#define STRINGIFY(A) #A

namespace tav
{
SNAudioPlane::SNAudioPlane(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "LitSphere"), texGridSize(64)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	pa = (PAudio*) scd->pa;
	osc = (OSCData*) scd->osc;

	// setup chanCols
	getChanCols(&chanCols);

	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];

	quadAr = new QuadArray(80, 80, -1.f, -1.f, 2.f, 2.f, 1.f, 1.f, 1.f, 1.f);

	quad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 1.f, 0.f,
			0.f, 1.f);

	waveRotModelMat = glm::mat4(1.f);
	normalMat = glm::mat3(glm::transpose(glm::inverse(waveRotModelMat)));

	posTex = new FBO(shCol, texGridSize, texGridSize, GL_RGB16F, GL_TEXTURE_2D,
			false, _scd->nrChannels, 1, 1, GL_CLAMP_TO_EDGE, false);
	normTex = new FBO(shCol, texGridSize, texGridSize, GL_RGB16F, GL_TEXTURE_2D,
			false, _scd->nrChannels, 1, 1, GL_CLAMP_TO_EDGE, false);

	// --- Texturen fuer den Litsphere Shader  ---

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
			((*scd->dataPath) + "textures/skybox_camch.png").c_str());

	addPar("alpha", &alpha);
	addPar("heightScale", &heightScale);

	initPreCalcShdr();
	stdTex = shCol->getStdTex();
}

//----------------------------------------------------

void SNAudioPlane::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (!inited)
	{
		initWaveShdr(_tfo);
		if (!_tfo)
			initRenderShdr();
		inited = true;
	}

	glClear (GL_DEPTH_BUFFER_BIT);
	glEnable (GL_CULL_FACE);
	glEnable (GL_DEPTH_TEST);
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// if there is a tfo record to it
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);
		_tfo->end(); // tfo muss angehalten werden, damit der shader gewechselt werden kann
		waveShdr->begin();
		_tfo->begin(GL_TRIANGLES);
		_tfo->enableDepthTest();

		sendStdShaderInit(waveShdr);
		useTextureUnitInd(0, tex0->getId(), waveShdr, _tfo);

		waveRotModelMat = glm::rotate(glm::mat4(1.f), float(M_PI) * -0.5f,
				glm::vec3(1.f, 0.f, 0.f));

		waveShdr->setUniformMatrix4fv("modelMatrix", &waveRotModelMat[0][0]);
		waveShdr->setUniformMatrix3fv("normalMatrix", &normalMat[0][0]);
		waveShdr->setUniformMatrix4fv("projectionMatrix",
				cp->multicam_projection_matrix);
		waveShdr->setUniform1i("posTex", 0);
		waveShdr->setUniform1i("normTex", 1);
		waveShdr->setUniform1f("time", float(time));

		glActiveTexture (GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, posTex->getColorImg());

		glActiveTexture (GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normTex->getColorImg());

		quadAr->draw(_tfo);

	}
	else
	{
//		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// if there is no tfo, render direct
		renderShdr->begin();

		renderShdr->setUniform1i("posTex", 0);
		renderShdr->setUniform1i("normTex", 1);
		renderShdr->setUniform1i("litSphereTex", 2);
		renderShdr->setUniform1i("cubeMap", 3);
		renderShdr->setUniform1f("brightness", osc->totalBrightness);
		renderShdr->setUniform1f("alpha", alpha * osc->alpha);
		renderShdr->setUniform1f("nrChannels", float(scd->nrChannels));
		renderShdr->setUniform1f("time", float(time));
		renderShdr->setUniform1f("heightScale", heightScale);

		litsphereTex->bind(2);
		cubeTex->bind(3);

		for (int i = 0; i < scd->nrChannels; i++)
		{
			//glClear(GL_DEPTH_BUFFER_BIT);

//			glm::mat4 m_vm = cp->view_matrix_mat4 * modelMat;
			glm::mat4 m_vm = cp->view_matrix_mat4
					* glm::translate(_modelMat,
							glm::vec3(0.f, 0.f,
									float(i) / float(scd->nrChannels) * -0.1f));
			glm::mat4 m_pvm = cp->projection_matrix_mat4 * m_vm;
			glm::mat3 normalMat = glm::mat3(
					glm::transpose(glm::inverse(_modelMat)));

			renderShdr->setUniformMatrix4fv("m_pvm", &m_pvm[0][0]);
			renderShdr->setUniformMatrix4fv("m_vm", &m_vm[0][0]);
			renderShdr->setUniformMatrix3fv("m_normal", &normalMat[0][0]);

			renderShdr->setUniform4fv("scnCol", &chanCols[scd->chanMap[i]][0]);

			glActiveTexture (GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, posTex->getColorImg(i));

			glActiveTexture (GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, normTex->getColorImg(i));

			quadAr->draw();
		}
	}

	if (_tfo)
	{
		_tfo->end(); // tfo muss angehalten werden, damit der shader gewechselt werden kann
		_shader->begin();
		_tfo->begin(GL_TRIANGLES);
	}
	else
	{
		// render with sceneNode shader
		//_shader->begin();
	}
}

//----------------------------------------------------

void SNAudioPlane::update(double time, double dt)
{
	scd->audioTex->update(pa->getPll(), pa->waveData.blockCounter);
	scd->audioTex->mergeTo2D(pa->waveData.blockCounter);

	// - pre calculate positions from audioTexture ----

	posTex->bind();
	posTex->clear();

	posTexShdr->begin();
	posTexShdr->setUniform1i("audioTex", 0);
	posTexShdr->setUniform1i("nrChannels", scd->nrChannels);
	posTexShdr->setUniform1f("time", time);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, scd->audioTex->getTex2D());

	quad->draw();
	posTex->unbind();

	// - pre calculate normals from precalculated positions -

	normTex->bind();
	normTex->clear();

	normTexShdr->begin();
	normTexShdr->setUniform1i("texGridSize", texGridSize);
	normTexShdr->setUniform1f("nrChannels", float(scd->nrChannels));

	for (int i = 0; i < scd->nrChannels; i++)
	{
		normTexShdr->setUniform1i(("pos_tex" + std::to_string(i)).c_str(), i);
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, posTex->getColorImg(i));
	}

	quad->draw();
	normTex->unbind();
}

//----------------------------------------------------

void SNAudioPlane::initPreCalcShdr()
{
	//- Position Shader ---

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position;\n layout (location=1) in vec3 normal;\n layout (location=2) in vec2 texCoord;\n layout (location=3) in vec4 color;\n out vec4 pos;\n out vec2 toTexPos;\n void main(void) {\n pos = position;\n toTexPos = texCoord;\n gl_Position = position;\n });
	vert = "// AudioPlane pos tex vertex shader\n" + shdr_Header + vert;

	std::string frag = "";
	for (int i = 0; i < scd->nrChannels; i++)
		frag += "layout(location = " + std::to_string(i) + ") out vec4 pos_tex"
				+ std::to_string(i) + ";\n";

	frag +=
			STRINGIFY(
					in vec4 pos;\n in vec2 toTexPos;\n uniform sampler2D audioTex;\n uniform int nrChannels;\n uniform float time;\n float ampX;\n float ampY;\n float amp2;\n float extZ = 10.0;\n int i;\n float chanOffs;\n

					vec4 getVal(int chanNr) { chanOffs = float(chanNr) / float(nrChannels) + 0.5 / float(nrChannels);\n ampX = texture(audioTex, vec2(toTexPos.x, chanOffs)).r * 0.5 + 0.5;\n
					//ampY = texture(audioTex, vec2(toTexPos.y, chanOffs)).r * 0.5 + 0.5;\n
					ampY += texture(audioTex, vec2(toTexPos.y - time * 0.1, chanOffs)).r * 0.5 + 0.5; amp2 = texture(audioTex, vec2(sqrt(toTexPos.x * toTexPos.x + toTexPos.y * toTexPos.y) * 0.5, chanOffs)).r;\n

					return vec4((pos.x + amp2 * 0.3), pos.y, (ampY + ampX) * -extZ + extZ - 1.0, 1.0);
					//return vec4(ampX);
					}\n \n void main() {\n );

	for (int i = 0; i < scd->nrChannels; i++)
		frag += "pos_tex" + std::to_string(i) + " = getVal(" + std::to_string(i)
				+ ");\n";

	frag += "}";

	frag = "// AudioPlane pos tex shader\n" + shdr_Header + frag;

	posTexShdr = shCol->addCheckShaderText("audioPlanePosTex", vert.c_str(),
			frag.c_str());

	//- Normal Shader ---

	vert =
			STRINGIFY(
					layout (location=0) in vec4 position;\n layout (location=1) in vec3 normal;\n layout (location=2) in vec2 texCoord;\n layout (location=3) in vec4 color;\n out vec2 toTexPos;\n void main(void) {\n toTexPos = vec2(position) * 0.5 + 0.5;\n gl_Position = position;\n });
	vert = "// AudioPlane norm tex vertex shader\n" + shdr_Header + vert;

	frag = "";
	for (int i = 0; i < scd->nrChannels; i++)
	{
		frag += "layout(location = " + std::to_string(i) + ") out vec4 norm_tex"
				+ std::to_string(i) + ";\n";
		frag += "uniform sampler2D pos_tex" + std::to_string(i) + ";\n";
	}

	frag +=
			STRINGIFY(
					in vec2 toTexPos;\n uniform int texGridSize;\n uniform int nrChannels;\n

					vec3 posTop;\n vec3 posBottom;\n vec3 posCenter;\n vec3 posLeft;\n vec3 posRight;\n vec3 norms[2];\n float gridOffs;\n \n vec4 getNorm (sampler2D inTex, float gOffs)\n {\n
					// read neighbour positions left, right, top, bottom
					posTop = texture(inTex, vec2(toTexPos.x, toTexPos.y + gOffs) ).xyz;\n posBottom = texture(inTex, vec2(toTexPos.x, toTexPos.y - gOffs)).xyz;\n posCenter = texture(inTex, toTexPos).xyz;\n posLeft = texture(inTex, vec2(toTexPos.x - gOffs, toTexPos.y)).xyz;\n posRight = texture(inTex, vec2(toTexPos.x + gOffs, toTexPos.y)).xyz;\n \n norms[0] = normalize(cross((posTop - posCenter), (posLeft - posCenter)));\n norms[1] = normalize(cross((posBottom - posCenter), (posRight - posCenter)));\n \n for(int i=0;i<2;i++){\n norms[i] = norms[i].z > 0.0 ? norms[i] : norms[i] * -1.0;\n }\n \n norms[0] = normalize((norms[0] + norms[1]) * 0.5);\n return vec4(norms[0], 1.0);\n }\n \n void main() {\n gridOffs = 1.0 / float(texGridSize);\n );

					for (int i=0; i<scd->nrChannels;i++)
					{
						frag += "norm_tex"+std::to_string(i)+" = getNorm( pos_tex"+std::to_string(i)+", gridOffs );\n";
					}

					frag += "}\n";
					frag = "// AudioPlane norm tex shader\n"+shdr_Header+frag;

					//std::cout << frag << std::endl;

					normTexShdr = shCol->addCheckShaderText("audioPlaneNormTex", vert.c_str(), frag.c_str());
				}

void SNAudioPlane::initWaveShdr(TFO* _tfo)
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert = STRINGIFY(
	layout (location=0) in vec4 position;
	layout (location=1) in vec3 normal;
	layout (location=2) in vec2 texCoord;
	layout (location=3) in vec4 color;

	uniform sampler2D posTex;
	uniform sampler2D normTex;

	uniform int useInstancing;
	uniform int texNr;
	uniform float time;

	uniform mat4 modelMatrix;
	uniform mat3 normalMatrix;
	uniform mat4 projectionMatrix;

	out vec4 rec_position;
	out vec3 rec_normal;
	out vec4 rec_texCoord;
	out vec4 rec_color;

	vec2 modTexCoord;

	void main(void) {
		modTexCoord = vec2(texCoord.x, texCoord.y - time * 0.1);
		rec_position = modelMatrix * texture(posTex, texCoord);
		rec_normal = normalize(normalMatrix * texture(normTex, texCoord).xyz);
		rec_color = color; rec_texCoord = vec4(modTexCoord, float(texNr), 0.0);
		gl_Position = rec_position;
	});
	vert = "// AudioPlane plane shader\n" + shdr_Header + vert;


	std::string frag = STRINGIFY(layout(location = 0)
		out vec4 color;
		void main() {
			color = vec4(1.0);
	});

	frag = "// AudioPlane plane shader\n" + shdr_Header + frag;

	waveShdr = shCol->addCheckShaderTextNoLink("audioPlaneFinal", vert.c_str(),
			frag.c_str());

	//- Setup TFO ---

	std::vector<std::string> names;
	// copy the standard record attribute names, number depending on hardware
	for (auto i = 0; i < MAX_SEP_REC_BUFS; i++)
		names.push_back(stdRecAttribNames[i]);
	_tfo->setVaryingsToRecord(&names, waveShdr->getProgram());

	waveShdr->link();
}

//----------------------------------------------------

void SNAudioPlane::initRenderShdr()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string stdVert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color;

					uniform sampler2D posTex;

					uniform mat4 m_vm; uniform mat4 m_pvm;

					uniform float heightScale; uniform float time;

					out TO_FS { vec2 tex_coord; vec2 quadPos; vec3 eye_pos; } vertex_out;

					void main() { vec2 modTexCoord = texCoord; vec4 pos = vec4(texture(posTex, texCoord).xyz, 1.0); pos.z *= heightScale;

					gl_Position = m_pvm * pos;

					vertex_out.quadPos = position.xy; vertex_out.tex_coord = modTexCoord; vertex_out.eye_pos = normalize( vec3( m_vm * pos ) ); });

	stdVert = "// SNAudioPlane vertex shader\n" + shdr_Header + stdVert;

	std::string frag =
			STRINGIFY(
					uniform sampler2D normTex; uniform sampler2D litSphereTex; uniform samplerCube cubeMap;

					uniform mat3 m_normal; uniform vec4 scnCol;

					in TO_FS { vec2 tex_coord; vec2 quadPos; vec3 eye_pos; } vertex_in;

					vec4 litColor; uniform float alpha; uniform float nrChannels; uniform float brightness;

					layout (location = 0) out vec4 color;

					void main() { vec3 n = normalize(m_normal * texture(normTex, vertex_in.tex_coord).xyz);

					// litsphere
					vec3 reflection = reflect( vertex_in.eye_pos, n );

					float m = 2.0 * sqrt( pow( reflection.x, 2.0 ) + pow( reflection.y, 2.0 ) + pow( reflection.z + 1.0, 2.0 ) ); vec2 vN = reflection.xy / m + 0.5;

					litColor = texture(litSphereTex, vN); float litBright = litColor.r * litColor.g * litColor.b;

					// irgendwie gibt es einen weissen rand, hack zum entfernen desselben
					float cut = (vertex_in.quadPos.x < -0.95 ? 0.0 : 1.0) * (vertex_in.quadPos.x > 0.95 ? 0.0 : 1.0); cut *= (vertex_in.quadPos.y < -0.95 ? 0.0 : 1.0) * (vertex_in.quadPos.y > 0.95 ? 0.0 : 1.0);

					if (cut == 0.0){ discard; } else { color = scnCol * litBright * brightness; color.a = alpha; } });

	frag = "// SNAudioPlane fragment shader\n" + shdr_Header + frag;

	renderShdr = shCol->addCheckShaderText("SNAudioPlane_rndr", stdVert.c_str(),
			frag.c_str());
}

//----------------------------------------------------

SNAudioPlane::~SNAudioPlane()
{
	delete posTex;
	delete normTex;
	delete quadAr;
	delete normTexShdr;
	delete posTexShdr;
	delete waveShdr;
}
}
