//
// SNCircleOrbitGPU.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNCircleOrbitGPU.h"

#define STRINGIFY(A) #A

using namespace glm;

namespace tav
{
SNCircleOrbitGPU::SNCircleOrbitGPU(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "LitSphere"), inited(false)
{
	pa = (PAudio*) scd->pa;
	osc = (OSCData*) scd->osc;
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	getChanCols(&chanCols);

	nrInst = 80;

	yAmp = 0.4f;
	circBaseSize = 1.5f;
	rotSpeed = 0.07f;
	depthScale = 20.0f;
	scaleAmt = 2.f;

	// make sure it´s the next higher power of two
	audioTex2DNrChans = float(int(float(scd->nrChannels / 2) + 0.5f)) * 2.f;

	// für jeden Circle mach Instanzes für jeden Kanal
	std::vector < coordType > instAttribs;
	circs = new Circle(80, 1.0f, 0.95f, float(M_PI) * 2.f, 1.f, 1.f, 1.f, 1.f,
			&instAttribs, nrInst);
}

//----------------------------------------------------

SNCircleOrbitGPU::~SNCircleOrbitGPU()
{
}

//----------------------------------------------------

void SNCircleOrbitGPU::draw(double time, double dt, camPar* cp,
		Shaders* _shader, TFO* _tfo)
{
	if (!inited)
	{
		initRecShdr(_tfo);
		inited = true;
	}

	if (_tfo)
	{
		timeIncr += float(dt * osc->speed * 0.8f);

		_tfo->end(); // tfo muss angehalten werden, damit der shader gewechselt werden kann

		recShdr->begin();

		sendStdShaderInit(recShdr);
		useTextureUnitInd(0, scd->audioTex->getTex2D(), recShdr, _tfo);

		_tfo->begin(GL_TRIANGLES);
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);

		recShdr->setUniform1f("nrChannels", audioTex2DNrChans);
		recShdr->setUniform1f("nrInst", float(nrInst));
		recShdr->setUniform1f("nrInstPerChan",
				float(nrInst) / float(scd->nrChannels));
		recShdr->setUniform4fv("audioCol", &chanCols[scd->chanMap[0]][0],
				scd->nrChannels);
		recShdr->setUniform1i("audioTex", 0);
		recShdr->setUniform1f("time", timeIncr);

		glActiveTexture (GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, scd->audioTex->getTex2D());

		circs->drawInstanced(nrInst, _tfo, 1.f);
	}
}

//----------------------------------------------------

void SNCircleOrbitGPU::initRecShdr(TFO* _tfo)
{
	std::string shdr_Header =
			"#version 410 core\n#pragma optimize(on)\n layout (location=0) in vec4 position; \nlayout (location=1) in vec3 normal;\nlayout (location=2) in vec2 texCoord;\nlayout (location=3) in vec4 color;\n uniform vec4 audioCol["
					+ std::to_string(scd->nrChannels) + "];\n";

	std::string vert =
			STRINGIFY(
					uniform float nrChannels; uniform float nrInst; uniform float nrInstPerChan; uniform sampler2D audioTex; uniform float time;

					out vec4 rec_position; out vec3 rec_normal; out vec4 rec_texCoord; out vec4 rec_color;

					float rotZ; float offsX; float offsY; float offsZ; float scale; float pi = 3.1415926535897932384626433832795; float amp; float amp2; float amp3;

					float angle; float chanInstID; float chanOffs; int chanNr;

					vec4 column0; vec4 column1; vec4 column2; vec4 column3;

					mat4 scale_matrix; mat4 trans_matrix;

					void main(void) { chanNr = int( float(gl_InstanceID) / nrInstPerChan ); chanInstID = mod( float(gl_InstanceID), nrInstPerChan );

					offsX = 0.0; offsY = 0.0; offsZ = mod(chanInstID / nrInstPerChan + time * 0.1, 1.0); offsZ += (float(chanNr) / nrChannels) * (1.0 / nrInstPerChan);

					angle = atan(position.y, position.x) / (2.0 * pi);

					chanOffs = float(chanNr) / nrChannels;

					amp = texture(audioTex, vec2(offsZ, chanOffs)).r * 2.0; amp2 = texture(audioTex, vec2(offsZ * 0.5, chanOffs)).r; amp3 = texture(audioTex, vec2(angle * 0.5, chanOffs)).r;

					rotZ = (chanInstID / nrInstPerChan + time * 0.001) * pi * 16.0; rotZ += pi * 2.0 * amp; rotZ += float(chanNr) / nrChannels * pi;

					rec_color = audioCol[chanNr]; rec_color.a = offsZ < 0.125 ? min(offsZ * 8.0, 1.0) : (1.0 - offsZ); rec_color.a *= (amp2 * 0.5 + 0.5) * 2.0;

					offsZ *= -6.0; offsZ += 1.0; scale = amp3 * 2.0 + 1.0;

					column0 = vec4(cos(rotZ), sin(rotZ), 0.0, 0.0); column1 = vec4(-sin(rotZ), cos(rotZ), 0.0, 0.0); column2 = vec4(0.0, 0.0, 1.0, 0.0); column3 = vec4(offsX, offsY, offsZ, 1.0); trans_matrix = mat4(column0, column1, column2, column3);

					column0 = vec4(scale, 0.0, 0.0, 0.0); column1 = vec4(0.0, scale, 0.0, 0.0); column2 = vec4(0.0, 0.0, scale, 0.0); column3 = vec4(0.0, 0.0, 0.0, 1.0); scale_matrix = mat4(column0, column1, column2, column3);

					rec_texCoord = vec4(texCoord, 1.f, 0.0);
//                                         rec_texCoord = vec4(texCoord, float(texNr) / nrChannels + (0.5f / nrChannels), 0.0);
					rec_position = trans_matrix * scale_matrix * position;
					//rec_normal = normalize(normalMatrix * normal);
					rec_normal = normal;

					gl_Position = rec_position;
//                                        gl_Position = trans_matrix * position;
					});
	vert = "// SNCircleOrbitGPU record shader\n" + shdr_Header + vert;

	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 color; void main() { color = vec4(1.0); });
	frag = "// SNCircleOrbitGPU record shader\n" + shdr_Header + frag;

	recShdr = shCol->addCheckShaderTextNoLink("circleOrbitGPURec", vert.c_str(),
			frag.c_str());

	//- Setup TFO ---

	std::vector<std::string> names;
	// copy the standard record attribute names, number depending on hardware
	for (auto i = 0; i < MAX_SEP_REC_BUFS; i++)
		names.push_back(stdRecAttribNames[i]);
	_tfo->setVaryingsToRecord(&names, recShdr->getProgram());

	recShdr->link();
}

//----------------------------------------------------

void SNCircleOrbitGPU::update(double time, double dt)
{
	scd->audioTex->mergeTo2D(pa->waveData.blockCounter);
	scd->audioTex->update(pa->getPll(), pa->waveData.blockCounter);
}

}
