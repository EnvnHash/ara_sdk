//
// SNAudioWaveGPU.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNAudioWaveGPU.h"

namespace tav
{

SNAudioWaveGPU::SNAudioWaveGPU(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "LitSphere"), ampScale(8.f), nrInstances(
				20), nrPar(3)
{
	pa = (PAudio*) scd->pa;
	oscData = (OSCData*) scd->osc;
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	getChanCols(&chanCols);

	signals = new float[nrPar];
	for (auto i = 0; i < nrPar; i++)
		signals[i] = 0.f;

	pllOffs = new float[nrPar];
	pllOffs[0] = 0.1f;
	pllOffs[1] = 0.2f;
	pllOffs[2] = 0.3f;

	nrSegmentsPerLine = pa->frames_per_buffer;

	glm::mat4 transMatr[4] = { glm::mat4(1.f), glm::mat4(1.f), glm::mat4(1.f), glm::mat4(1.f) };

	// right side
	transMatr[0] = glm::translate(transMatr[0], glm::vec3(0.f, -1.f, 0.f));

	transMatr[1] = glm::translate(transMatr[1], glm::vec3(-1.f, 0.f, 0.f));
	transMatr[1] = glm::rotate(transMatr[1], static_cast<float>(M_PI) * -0.5f,
			glm::vec3(0.f, 0.f, 1.f));

	transMatr[2] = glm::translate(transMatr[2], glm::vec3(1.f, 0.f, 0.f));
	transMatr[2] = glm::rotate(transMatr[2], static_cast<float>(M_PI) * 0.5f,
			glm::vec3(0.f, 0.f, 1.f));

	lines = new Line*[nrChan];
	pllWaves = new TextureBuffer*[nrChan];

	tMatrs = new glm::mat4[nrChan];
	for (int i = 0; i < nrChan; i++)
	{
		lines[i] = new Line(nrSegmentsPerLine, 1.f, 1.f, 1.f, 1.f);
		pllWaves[i] = new TextureBuffer(nrSegmentsPerLine, 1);
		tMatrs[i] = transMatr[i % 4];
	}

	std::string header = "#version 410\n";
	vs = header + STRINGIFY(layout(location=0) in vec4 position;
	layout(location=1) in vec4 normal;
	layout(location=2) in vec2 texCoord;
	layout(location=3) in vec4 color;

	out VS_GS_VERTEX {
		vec4 position;
	} vertex_out;

	void main() {
		vertex_out.position = position;
		gl_Position = vertex_out.position;
	});


	vs2 = header + STRINGIFY(layout(location=0) in vec4 position;
	layout(location=1) in vec4 normal;
	layout(location=2) in vec2 texCoord;
	layout(location=3) in vec4 color;

	uniform samplerBuffer pllBuf;
	uniform int nrSamplesPerBuf;
	uniform mat4 m_pvm;

	out VS_GS_VERTEX { vec4 position; } vertex_out;

	void main() {
		vec4 posL = texelFetch(pllBuf, gl_VertexID);
		vertex_out.position = m_pvm * vec4(position.x, sqrt( abs(position.y + posL.x) ), position.z, position.w);
		gl_Position = vertex_out.position;
	});


	// convert points to quads
	gs = header + STRINGIFY(#version 410\n
	layout (lines) in; layout (triangle_strip, max_vertices = 4) out;

	uniform mat4 rotVec; // for rotating the line vektor
	uniform mat4 m_pvm;
	uniform samplerBuffer pllBuf;
	uniform int nrSamplesPerBuf;
	uniform float nrInstances;
	uniform float quadHeight;

	const float pi = 3.14159265;

	in VS_GS_VERTEX { vec4 position; } vertex_in[];

	out vec4 rec_position; out vec4 rec_color;

	void main() {
		if (gl_PrimitiveIDIn+1 >= nrSamplesPerBuf) return;

		// pll Value
		vec4 posL = texelFetch(pllBuf, gl_PrimitiveIDIn);
		posL = m_pvm * vec4(gl_in[0].gl_Position.x, sqrt( (gl_in[0].gl_Position.y + posL.x) ), gl_in[0].gl_Position.z, gl_in[0].gl_Position.w);

		vec4 posR = texelFetch(pllBuf, (gl_PrimitiveIDIn+1));
		posR = m_pvm * vec4(gl_in[1].gl_Position.x, sqrt( (gl_in[1].gl_Position.y + posR.x) ), gl_in[1].gl_Position.z, gl_in[1].gl_Position.w);

		vec4 vecToNext = normalize(rotVec * (posL - posR));

		// left lower corner of the quad
		rec_position = posL - vecToNext * quadHeight; rec_color = vec4(1.0, 1.0, 1.0, 1.0); gl_Position = rec_position; EmitVertex();

		// left upper corner of the quad
		rec_position = posL + vecToNext * quadHeight; rec_color = vec4(1.0, 1.0, 1.0, 1.0); gl_Position = rec_position; EmitVertex();

		// right upper corner of the quad
		rec_position = posR - vecToNext * quadHeight; rec_color = vec4(1.0, 1.0, 1.0, 1.0); gl_Position = rec_position; EmitVertex();

		// right lower corner of the quad
		rec_position = posR + vecToNext * quadHeight; rec_color = vec4(1.0, 1.0, 1.0, 1.0); gl_Position = rec_position; EmitVertex();

		EndPrimitive();
	});

	fs = header+STRINGIFY(
			layout (location = 0) out vec4 color;
	void main() { color = vec4(1.0); });

	waveShader = new Shaders(vs2.c_str(), fs.c_str(), false);

	/*
	 std::vector<string>  recAttribNames;
	 recAttribNames.push_back(stdRecAttribNames[tav::POSITION]);
	 recAttribNames.push_back(stdRecAttribNames[tav::COLOR]);

	 intTfo = new TFO(nrSegmentsPerLine * nrInstances, recAttribNames);
	 intTfo->setVaryingsToRecord(&recAttribNames, waveShader->getProgram());
	 */
	waveShader->link();

	rotVecMat = glm::rotate(glm::mat4(1.f), static_cast<float>(M_PI * 0.5f),
			glm::vec3(0.f, 0.f, 1.f));
	quadAr = new QuadArray(30, 30, -1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f));
	quad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 1.f);
	recFbo = new FBO(shCol, 512, 512);
	blur = new FastBlur(oscData, shCol, 512, 512);
}

//----------------------------------------------------

SNAudioWaveGPU::~SNAudioWaveGPU()
{
	waveShader->remove();
	delete waveShader;
	delete[] pllWaves;
}

//----------------------------------------------------

void SNAudioWaveGPU::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);
	}

	sendStdShaderInit(_shader);
	useTextureUnitInd(0, blur->getResult(), _shader, _tfo);
	quadAr->draw(_tfo);
}

//----------------------------------------------------

void SNAudioWaveGPU::update(double time, double dt)
{
	glActiveTexture (GL_TEXTURE0);

	// upload waveforms
	if (lastBlock != pa->waveData.blockCounter)
	{
		for (auto i = 0; i < nrChan; i++)
		{
			GLfloat* pllPtr = pllWaves[i]->getMapBuffer();

			for (auto j = 0; j < pa->frames_per_buffer; j++)
			{
				float fInd = static_cast<float>(j)
						/ static_cast<float>(pa->frames_per_buffer - 1);
				*pllPtr = pa->getPllAtPos(i, fInd);
				pllPtr++;
			}

			pllWaves[i]->unMapBuffer();
		}

		lastBlock = pa->waveData.blockCounter;
	}

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glEnable(GL_RASTERIZER_DISCARD);

	recFbo->bind();
	recFbo->clear();

	waveShader->begin();
	waveShader->setUniform1i("pllBuf", 0);
	waveShader->setUniform1i("nrSamplesPerBuf", pa->frames_per_buffer);

	/*        waveShader->setUniform1f("nrInstances", static_cast<float>(nrInstances));
	 waveShader->setUniform1f("quadHeight", 0.005f);
	 waveShader->setUniformMatrix4fv("rotVec", &rotVecMat[0][0]);

	 intTfo->bind();
	 intTfo->begin(GL_TRIANGLES);
	 */

	for (auto i = 0; i < nrChan; i++)
	{
		waveShader->setUniformMatrix4fv("m_pvm", &tMatrs[i][0][0]);
		glBindTexture(GL_TEXTURE_BUFFER, pllWaves[i]->getTex());
		lines[i]->draw(GL_LINE_STRIP, nullptr, GL_LINE_STRIP);
//            lines[i]->draw(GL_LINE_STRIP, intTfo, GL_TRIANGLES);
		//lines[i]->drawInstanced(GL_LINE_STRIP, nrInstances, intTfo, GL_TRIANGLES);
	}

	waveShader->end();
//        
//        intTfo->end();
//        intTfo->unbind();

	recFbo->unbind();
	//glDisable(GL_RASTERIZER_DISCARD);

	blur->proc(recFbo->getColorImg());

}

}
