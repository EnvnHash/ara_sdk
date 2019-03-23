//
// SNImageFFT.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNImageFFT.h"

#define STRINGIFY(A) #A

namespace tav
{
SNImageFFT::SNImageFFT(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "NoLight"), sendInt(2), sendCtr(0)
{
	osc_handler = static_cast<OSCHandler*>(scd->oscHandler);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	quad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 1.f, 0.f,
			0.f, 1.f);

	fftSize = 1024;

#ifdef WITH_AUDIO
	fft = new FFT(fftSize);
#endif

	renderFbo = new FBO(shCol, fftSize, fftSize, GL_RGBA8, GL_TEXTURE_2D, true,
			1, 1, 1, GL_CLAMP_TO_EDGE, false);

	// make a bufer for donwloading the fbo
	data = new unsigned char[fftSize * fftSize * 4];
	oneRow = new float[fftSize];
	tex = new TextureManager();
	tex->loadTexture2D((*scd->dataPath) + "textures/mann.jpg");

	initShdr();
	stdTex = shCol->getStdTex();
}

//---------------------------------------------------------------

SNImageFFT::~SNImageFFT()
{
	delete quad;
}

//---------------------------------------------------------------

void SNImageFFT::initShdr()
{
	//- Position Shader ---

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position;\n layout (location=1) in vec3 normal;\n layout (location=2) in vec2 texCoord;\n layout (location=3) in vec4 color;\n

					uniform mat4 m_pvm;\n out vec2 tex_coord;\n

					void main() {\n tex_coord = texCoord;\n gl_Position = m_pvm * position;\n });
	vert = "// ImageFFT Text Content vert shader\n" + shdr_Header + vert;

	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string frag =
			STRINGIFY(
					layout (location = 0) out vec4 FragColor;\n in vec2 tex_coord;\n float pi = 3.1415926535897932384626433832795; uniform float time; uniform sampler2D tex;

					void main()\n {\n
					//float col = sin( tex_coord.x * 2.0 * pi * 20.0 * (mod(time, 1.0) + 1.0) ) * 0.5 + 0.5;\n
					//col *= min(1.0 - tex_coord.x, 0.1) / 0.1;
					vec4 col = texture(tex, vec2( tex_coord.x * sin(tex_coord.y + time) , tex_coord.y + sin(tex_coord.x + time*0.6)) ); FragColor = col;\n });
	frag = "// ImageFFT Text Content frag shader\n" + shdr_Header + frag;

	testShdr = shCol->addCheckShaderText("SNImage_text", vert.c_str(),
			frag.c_str());
}

//---------------------------------------------------------------

void SNImageFFT::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	//        sendStdShaderInit(_shader);
	//        sendPvmMatrix(_shader, cp);

	renderFbo->bind();

	testShdr->begin();
	testShdr->setIdentMatrix4fv("m_pvm");
	testShdr->setUniform1i("tex", 0);
	testShdr->setUniform1f("time", time);
//	testShdr->setUniform1f("time", 0.f);

	tex->bind(0);
	quad->draw(_tfo);

	renderFbo->unbind();

	//---- download fbo----

	glBindTexture(GL_TEXTURE_2D, renderFbo->getColorImg());
	glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, &data[0]);

	// control display ---

	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderFbo->getColorImg());

	quad->draw();

}

//---------------------------------------------------------------

void SNImageFFT::update(double time, double dt)
{
	int rowOffs = fftSize * 4 * (fftSize / 2); // jump to y-center of image

	// extract one row and convert it to black and white
	for (auto i = 0; i < fftSize; i++)
	{
		oneRow[i] = (float(data[rowOffs + i * 4])
				+ float(data[rowOffs + i * 4 + 1])
				+ float(data[rowOffs + i * 4 + 2])
				+ float(data[rowOffs + i * 4 + 3])) / 256.f * 2.f - 1.f;
//		oneRow[i] = ((float(data[rowOffs + i*4]) + float(data[rowOffs + i*4 +1]) + float(data[rowOffs + i*4 +2]) + float(data[rowOffs + i*4+3])) / (255.f*4.f)) * 2.f - 1.f;
//		std::cout << oneRow[i] << ", ";
	}

	//std::cout << std::endl;
	//std::cout << std::endl;

#ifdef WITH_AUDIO

	// do fft
	fft->setSignal(oneRow);
	fft->preparePolar();

	// send to SuperCollider
	if (sendCtr % sendInt == 0)
	{
		sendCtr = 0;
		osc_handler->sendFFT("127.0.0.1", "57120", "/mags", fft->getAmplitude(),
				fftSize / 2);
	}
#endif

	sendCtr++;
}

}
