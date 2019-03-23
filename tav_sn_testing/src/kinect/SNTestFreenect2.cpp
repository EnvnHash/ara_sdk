/*
 * SNTestFreenect2.cpp
 *
 *  Created on: 12.01.2016
 *      Copyright by Sven Hahne
 */

#include "SNTestFreenect2.h"
#define STRINGIFY(A) #A

namespace tav
{
SNTestFreenect2::SNTestFreenect2(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    				SceneNode(_scd, _sceneArgs)
{
	Freenect2In** allFnc = static_cast<Freenect2In**>(scd->fnc);
	fnc = allFnc[0];

	quad = scd->stdQuad;
	quad->rotate(M_PI, 0.f, 0.f, 1.f);

	initShaders();
}

//----------------------------------------------------

void SNTestFreenect2::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	sendStdShaderInit(_shader);
	assignTexUnits(_shader);
	if (scd->scnStructMode == BLEND) glActiveTexture(GL_TEXTURE0+4); else glActiveTexture(GL_TEXTURE0);

	renderGrayShader->begin();

	glBindTexture(GL_TEXTURE_2D, fnc->getDepthTexId(0));
	if (_tfo) _tfo->addTexture(0, fnc->getDepthTexId(0), GL_TEXTURE_2D, "texs");

	quad->draw(_tfo);
}

//----------------------------------------------------

void SNTestFreenect2::update(double time, double dt)
{
	fnc->uploadDepth(0);
}

//----------------------------------------------------

void SNTestFreenect2::initShaders()
{
	std::string shdr_Header = "#version 410\n\n";

	std::string vert =
			STRINGIFY(layout( location = 0 ) in vec4 position;\n
			layout( location = 1 ) in vec3 normal;\n
			layout( location = 2 ) in vec2 texCoord;\n
			layout( location = 3 ) in vec4 color;\n
			out vec2 TexCoord;\n
			void main(void) {\n
				TexCoord = texCoord;\n
				gl_Position = vec4(position.xy, 0.0, 1.0);\n
			});

	vert = shdr_Header + vert;

	std::string grayfrag =
			STRINGIFY(uniform sampler2D Data;\n
			vec4 tempColor;\n
			in vec2 TexCoord;\n
			layout(location = 0) out vec4 Color;\n

			void main(void) {\n
				tempColor = texture(Data, TexCoord);\n
				Color = vec4(tempColor.x/4500.0, tempColor.x/4500.0, tempColor.x/4500.0, 1);\n
			});

	grayfrag = shdr_Header + grayfrag;

	std::string frag =
			STRINGIFY(uniform sampler2D Data;\n
			in vec2 TexCoord;\n
			layout(location = 0) out vec4 Color;\n

			void main(void) {\n
				Color = texture(Data, TexCoord);\n
			});

	frag = shdr_Header + frag;

	renderShader = shCol->addCheckShaderText("Freenect2In", vert.c_str(),
			frag.c_str());

	renderGrayShader = shCol->addCheckShaderText("Freenect2InGray",
			vert.c_str(), grayfrag.c_str());
}

//----------------------------------------------------

SNTestFreenect2::~SNTestFreenect2()
{
	delete quad;
}
}
