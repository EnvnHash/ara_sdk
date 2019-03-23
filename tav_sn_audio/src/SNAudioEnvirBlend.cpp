//
// SNAudioEnvirBlend.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNAudioEnvirBlend.h"

#define STRINGIFY(A) #A

using namespace std;

namespace tav
{

SNAudioEnvirBlend::SNAudioEnvirBlend(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "PerlinAudioSphere")
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	pa = (PAudio*) scd->pa;
	osc = (OSCData*) scd->osc;

	// setup chanCols
	getChanCols(&chanCols);

	sphere = new Sphere(2.f, 60, false);

	noiseTex = new Noise3DTexGen(shCol, false, 1, 256, 256, 64, 4.f, 4.f, 16.f);

	audioTex2DNrChans = int(float(scd->nrChannels / 2) + 0.5f) * 2; // make sure it´s power of two

	rotAxisY = new Median<float>*[scd->nrChannels];
	rotAxisZ = new Median<float>*[scd->nrChannels];
	intModelMat = new glm::mat4[scd->nrChannels];
	normalMat = new glm::mat3[scd->nrChannels];

	for (short i = 0; i < scd->nrChannels; i++)
	{
		rotAxisY[i] = new Median<float>(8.f);
		rotAxisZ[i] = new Median<float>(8.f);
	}

	stdTex = shCol->getStdTex();

	quad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 1.f, 0.f,
			0.f, 1.f);

	addPar("alpha", &alpha);
}

//----------------------------------------------------

void SNAudioEnvirBlend::draw(double time, double dt, camPar* cp,
		Shaders* _shader, TFO* _tfo)
{
	float newRotX, newRotZ;

	if (!inited)
	{
		if (_tfo)
			initRecShdr(_tfo);
		else
			initShdr(cp);

		audioCol = new glm::vec4[audioTex2DNrChans];
		audioCol[1] = glm::vec4(0.05f, 0.1f, 0.6f, 1.f);
		audioCol[0] = glm::vec4(1.f, 1.f, 0.f, 1.f);
		multiModelMat = new glm::mat4*[audioTex2DNrChans];
		for (short i = 0; i < audioTex2DNrChans; i++)
			multiModelMat[i] = new glm::mat4[cp->nrCams];

		inited = true;
	}

	if (_tfo)
	{
		_tfo->end(); // tfo muss angehalten werden, damit der shader gewechselt werden kann

		recShdr->begin();
		sendStdShaderInit(recShdr);
		useTextureUnitInd(0, scd->audioTex->getTex2D(), recShdr, _tfo);
		_tfo->addTexture(1, noiseTex->getTex(), GL_TEXTURE_3D, "perlinTex");
		_tfo->begin(GL_TRIANGLES);
		_tfo->disableDepthTest();
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);
		_tfo->setSceneNodeColors(chanCols);

	}
	else
	{
		sphereShdr->begin();
	}

	/*
	 stdTex->begin();
	 stdTex->setIdentMatrix4fv("m_pvm");
	 stdTex->setUniform1i("tex", 0);
	 glActiveTexture(GL_TEXTURE0);
	 glBindTexture(GL_TEXTURE_2D, scd->audioTex->getTex2D());
	 quad->draw();
	 */

	glEnable (GL_BLEND);
	glDisable (GL_CULL_FACE);
	glDisable (GL_DEPTH_TEST);

	for (short i = 0; i < audioTex2DNrChans; i++)
	{

		float fInd = float(i) / std::max(float(audioTex2DNrChans), 1.f);

		newRotX = pa->getPllAtPos(scd->chanMap[i], 0.1f) * 0.5f;
		newRotZ = pa->getPllAtPos(scd->chanMap[i], 0.2f) * 0.5f;

		rotAxisY[i]->update(newRotX);
		rotAxisZ[i]->update(newRotZ);

		normalMat[i] = glm::mat3(glm::transpose(glm::inverse(intModelMat[i])));

		if (!_tfo)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			//  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			for (short j = 0; j < cp->nrCams; j++)
			{
				// multiModelMat[i][j] = modelMat;

				multiModelMat[i][j] = glm::rotate(_modelMat,
						pa->getPllAtPos(scd->chanMap[i], 0.1f) * 0.5f
						//+ float(time * 0.1)
								+ float(M_PI) * -0.2f + fInd * 0.5f,
						glm::normalize(
								glm::vec3(fInd + 0.2f, rotAxisY[i]->get(),
										rotAxisZ[i]->get())));
			}

			sphereShdr->setUniformMatrix4fv("model_matrix_g",
					&multiModelMat[i][0][0][0], cp->nrCams);
			sphereShdr->setUniformMatrix4fv("view_matrix_g",
					cp->multicam_view_matrix, cp->nrCams);
			sphereShdr->setUniformMatrix4fv("projection_matrix_g",
					cp->projection_matrix, cp->nrCams);

			sphereShdr->setUniform1i("audioTex", 0);
			sphereShdr->setUniform1i("perlinTex", 1);
			sphereShdr->setUniform1f("alpha", alpha * osc->alpha);
			sphereShdr->setUniform1f("brightness", osc->totalBrightness);
			sphereShdr->setUniform4f("audioCol", chanCols[scd->chanMap[i]].r,
					chanCols[scd->chanMap[i]].g, chanCols[scd->chanMap[i]].b,
					chanCols[scd->chanMap[i]].a);

			sphereShdr->setUniform1f("audioTexOffs",
					float(i) / float(audioTex2DNrChans)
							+ 0.5f / float(audioTex2DNrChans));

			glActiveTexture (GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, scd->audioTex->getTex2D());

			glActiveTexture (GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_3D, noiseTex->getTex());

			sphere->draw();
//            quad->draw();

		}
		else
		{
			intModelMat[i] = glm::rotate(glm::mat4(1.f),
					pa->getPllAtPos(scd->chanMap[i], 0.1f) * 0.5f
							+ float(time * 0.1) + fInd * 0.5f,
					glm::normalize(
							glm::vec3(fInd + 0.2f, rotAxisY[i]->get(),
									rotAxisZ[i]->get())));

			recShdr->setUniformMatrix4fv("modelMatrix", &intModelMat[i][0][0]);
			recShdr->setUniformMatrix3fv("normalMatrix", &normalMat[i][0][0]);
			recShdr->setUniform1i("texNr", scd->chanMap[i]);
			recShdr->setUniform1f("nrChannels", audioTex2DNrChans);
			recShdr->setUniform4fv("audioCol", &chanCols[scd->chanMap[i]][0]);
			sphere->draw(_tfo);
		}
	}
}

//----------------------------------------------------

void SNAudioEnvirBlend::update(double time, double dt)
{
	scd->audioTex->update(pa->getPll(), pa->waveData.blockCounter);
	scd->audioTex->mergeTo2D(pa->waveData.blockCounter);
}

//----------------------------------------------------

void SNAudioEnvirBlend::initShdr(camPar* cp)
{
	std::string nrCams = std::to_string(cp->nrCams);

	//- Position Shader ---

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=1) in vec3 normal; layout (location=2) in vec2 texCoord; layout (location=3) in vec4 color; uniform int camNr;
					// out vec4 pos;
					out vec2 texCo;

					void main(void) {
					//  pos = position;
					texCo = texCoord; gl_Position = position;
//                                     gl_Position = projectionMatrix[camNr] * modelMatrix[camNr] * position;
					});
	vert = "// SNAudioEnvirBlend pos tex vertex shader\n" + shdr_Header + vert;

	shdr_Header =
			"#version 410 core\n#pragma optimize(on)\n layout(triangles, invocations="
					+ nrCams
					+ ") in;\n layout(triangle_strip, max_vertices=3) out;\n uniform mat4 model_matrix_g["
					+ nrCams + "];uniform mat4 view_matrix_g[" + nrCams
					+ "];uniform mat4 projection_matrix_g[" + nrCams + "];";

	std::string geom =
			STRINGIFY(
					in vec2 texCo[]; out vec2 gs_texCo; void main() { for (int i=0; i<gl_in.length(); i++) { gl_ViewportIndex = gl_InvocationID; gs_texCo = texCo[i]; gl_Position = projection_matrix_g[gl_InvocationID] * view_matrix_g[gl_InvocationID] * model_matrix_g[gl_InvocationID] * gl_in[i].gl_Position; EmitVertex(); } EndPrimitive(); });

	geom = "// SNAudioEnvirBlend pos tex geom shader\n" + shdr_Header + geom;

	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 fragColor; in vec2 gs_texCo; uniform sampler2D audioTex; uniform sampler3D perlinTex; uniform vec4 audioCol; uniform float audioTexOffs; uniform float alpha; uniform float brightness;

					float pi = 3.1415926535897932384626433832795; float amp; float amp2; float amp3; float newNoise = 0.0;

					void main() {
					//amp3 = texture(audioTex, vec2(gs_texCo.x, audioTexOffs)).r * 0.5 + 0.5;

					amp3 = texture(audioTex, vec2(gs_texCo.x * 0.2, audioTexOffs)).r * 0.5 + 0.5; amp2 = texture(audioTex, vec2(gs_texCo.y, audioTexOffs)).r * 0.5 + 0.5; newNoise = texture(perlinTex, vec3(gs_texCo * amp3, amp2) ).r; amp = sqrt( texture(audioTex, vec2(2.0 * gs_texCo.x * newNoise, audioTexOffs)).r * 0.5 + 0.5 ); fragColor = audioCol * vec4(abs(amp * amp2 * 2.0) * 0.9) * 1.3;

					fragColor *= fragColor; fragColor *= brightness; fragColor.a *= 0.8 * alpha;
					// fragColor = audioCol;

					});
	frag = "// SNAudioEnvirBlend pos tex shader\n" + shdr_Header + frag;

	sphereShdr = shCol->addCheckShaderText("audioEnvirBlend", vert.c_str(),
			geom.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNAudioEnvirBlend::initRecShdr(TFO* _tfo)
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=1) in vec3 normal; layout (location=2) in vec2 texCoord; layout (location=3) in vec4 color;

					uniform float nrChannels; uniform int texNr; uniform vec4 audioCol;

					uniform mat4 modelMatrix; uniform mat3 normalMatrix;

					out vec4 rec_position; out vec3 rec_normal; out vec4 rec_texCoord; out vec4 rec_color;

					void main(void) { rec_color = audioCol; rec_texCoord = vec4(texCoord, float(texNr) / nrChannels, 0.0); rec_position = modelMatrix * position; rec_normal = normalize(normalMatrix * normal);

					gl_Position = modelMatrix * position; });
	vert = "// SNAudioEnvirBlend record shader\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 color; void main() { color = vec4(1.0); });
	frag = "// SNAudioEnvirBlend record shader\n" + shdr_Header + frag;

	recShdr = shCol->addCheckShaderTextNoLink("audioEnvirBlendRec",
			vert.c_str(), frag.c_str());

	//- Setup TFO ---

	std::vector<std::string> names;
	// copy the standard record attribute names, number depending on hardware
	for (auto i = 0; i < MAX_SEP_REC_BUFS; i++)
		names.push_back(stdRecAttribNames[i]);
	_tfo->setVaryingsToRecord(&names, recShdr->getProgram());

	recShdr->link();
}

//----------------------------------------------------

void SNAudioEnvirBlend::onKey(int key, int scancode, int action, int mods)
{
}

//----------------------------------------------------

SNAudioEnvirBlend::~SNAudioEnvirBlend()
{
	delete sphere;
}

}
