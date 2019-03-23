//
// GodRays.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "GodRays.h"

#define STRINGIFY(A) #A

namespace tav
{
GodRays::GodRays(ShaderCollector* _shCol, unsigned int _width,
		unsigned int _height, unsigned int _nrSamples, GLenum _type) :
		shCol(_shCol), width(_width), height(_height), exposure(0.001f), decay(
				0.9999f), density(0.084f), weight(5.65f), alpha(1.f), nrSamples(
				_nrSamples)
{
	quad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 1.f, nullptr, 1, true);

	quadNoFlip = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f,
			0.f, 0.f, 1.f, nullptr, 1, false);

	lightCircle = new Circle(20, 0.1f, 0.f);
	lightPos = glm::vec3(0.8f, 0.4f, 0.f);

	godRaysFbo = new FBO(shCol, width, height, _type, GL_TEXTURE_2D, false, 1,
			0, 1, GL_CLAMP_TO_EDGE, false);

	stdColShdr = shCol->getStdCol();
	stdTex = shCol->getStdTexAlpha();

	lightPosScreen = glm::vec2(0.f, 0.f);

	initShdr();
}

//----------------------------------------------------

void GodRays::initShdr()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string vert =
			STRINGIFY(
					layout(location = 0) in vec4 position;\n layout(location = 1) in vec4 normal;\n layout(location = 2) in vec2 texCoord;\n

					uniform mat4 m_pvm;\n uniform vec2 lightPositionOnScreen;\n uniform float density;\n);

	vert += "const int NUM_SAMPLES =" + std::to_string(nrSamples) + ";\n";

	vert +=
			STRINGIFY(
					out toFrag{\n vec2 texCoord;\n vec2 deltaTextCoord;\n } vertex_out;\n

					void main() {\n vertex_out.deltaTextCoord = vec2(texCoord - lightPositionOnScreen.xy );\n vertex_out.deltaTextCoord *= 1.0 / float(NUM_SAMPLES) * density;\n vertex_out.texCoord = texCoord;\n gl_Position = m_pvm * position;\n });

	vert = "// GodRayShdr Shader\n" + shdr_Header + vert;

	//------------------------------------------------------------------------

	std::string frag =
			STRINGIFY(
					layout (location = 0) out vec4 color;\n

					uniform sampler2D firstPass;\n uniform float exposure;\n uniform float decay;\n uniform float weight;\n uniform float alpha;\n);

	frag += "const int NUM_SAMPLES =" + std::to_string(nrSamples) + ";\n";

	frag +=
			STRINGIFY(
					\n in toFrag {\n vec2 texCoord;\n vec2 deltaTextCoord;\n } vertex_in;\n

					void main() {\n vec2 textCoo = vertex_in.texCoord;\n float illuminationDecay = 1.0;\n vec4 outCol = vec4(0.0);\n

					for(int i=0; i < NUM_SAMPLES ; i++) {\n textCoo -= vertex_in.deltaTextCoord;\n vec4 sampleCol = texture(firstPass, textCoo );\n sampleCol *= illuminationDecay * weight;\n outCol += sampleCol;\n illuminationDecay *= decay;\n }\n

					outCol *= exposure;\n

					float bright = (outCol.r + outCol.g + outCol.b) * 0.33333;\n
					//outCol.a *= bright;\n
					outCol.a *= alpha;\n

					if (outCol.a < 0.02) discard;\n

					color = outCol;\n });

	frag = "// SNIconParticles GodRayShdr Shader\n" + shdr_Header + frag;

	godRayShdr = shCol->addCheckShaderText("godRayShdr", vert.c_str(),
			frag.c_str());
}

//----------------------------------------------------

void GodRays::bind()
{
	// draw net to fbo
	godRaysFbo->bind();
//	glClear(GL_COLOR_BUFFER_BIT);
//	glClearColor(0.f, 0.f, 0.f, 1.f);
	godRaysFbo->clear();
}

//----------------------------------------------------

void GodRays::unbind()
{
	godRaysFbo->unbind();
}

//----------------------------------------------------

void GodRays::draw()
{
	//glm::vec2 lightPosScreen = glm::vec2(getOscPar("lightX"), getOscPar("lightY"));

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	godRayShdr->begin();
	godRayShdr->setIdentMatrix4fv("m_pvm");
	godRayShdr->setUniform1i("firstPass", 0);

	godRayShdr->setUniform1f("exposure", exposure);
	godRayShdr->setUniform1f("decay", decay);
	godRayShdr->setUniform1f("density", density);
	godRayShdr->setUniform1f("alpha", alpha);
	godRayShdr->setUniform1f("weight", weight);
	godRayShdr->setUniform2f("lightPositionOnScreen", lightPosScreen.x,
			lightPosScreen.y);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, godRaysFbo->getColorImg());

	quadNoFlip->draw();

	godRayShdr->end();
}

//----------------------------------------------------

void GodRays::drawLight(glm::vec4* lightCol)
{
	stdColShdr->begin();
	stdColShdr->setIdentMatrix4fv("m_pvm");
	lightCircle->setColor(lightCol->r, lightCol->g, lightCol->b, lightCol->a);
	lightCircle->draw();
}

//----------------------------------------------------

void GodRays::setAlpha(float _val)
{
	alpha = _val;
}

//----------------------------------------------------

void GodRays::setExposure(float _val)
{
	exposure = _val;
}

//----------------------------------------------------

void GodRays::setDensity(float _val)
{
	density = _val;
}

//----------------------------------------------------

void GodRays::setDecay(float _val)
{
	decay = _val;
}

//----------------------------------------------------

void GodRays::setWeight(float _val)
{
	weight = _val;
}

//----------------------------------------------------

void GodRays::setLightPosScr(float _x, float _y)
{
	lightPosScreen.x = _x;
	lightPosScreen.y = _y;
}

//----------------------------------------------------

GodRays::~GodRays()
{
	delete quad;
	delete quadNoFlip;
	delete lightCircle;
	delete godRaysFbo;
}

}
