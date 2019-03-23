//
//  SNTexMosaicShuffler.cpp
//  tav_scene
//
//  Created by Sven Hahne on 09/03/16.
//  Copyright © 2016 Sven Hahne. All rights reserved.
//

#include "SNTexMosaicShuffler.h"

#define STRINGIFY(A) #A

namespace tav
{
SNTexMosaicShuffler::SNTexMosaicShuffler(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "DirLight")
{
	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];

	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	quad = new Quad(-1.f, -1.f, 2.f, 2.f);

	addPar("ySinScale", &ySinScale);
	addPar("ySinFreq", &ySinFreq);
	addPar("ySinScale2", &ySinScale2);
	addPar("ySinFreq2", &ySinFreq2);
	addPar("ySinScale3", &ySinScale3);
	addPar("ySinFreq3", &ySinFreq3);
	addPar("ySinScale4", &ySinScale4);
	addPar("ySinFreq4", &ySinFreq4);

	gridSizeX = 18;
	gridSizeY = 12;
	nrIds = gridSizeX * gridSizeY;

	// make ordered list
	std::vector<int> ids;
	for (int i=0;i<nrIds;i++) ids.push_back(i);

	randIds = new int[nrIds];
	sizeMod = new glm::vec2[nrIds];
	for (int i=0;i<nrIds;i++)
	{
		unsigned int randInd = static_cast<unsigned int>(getRandF(0.f, (float)ids.size()));
		randIds[i] = ids[randInd];
		ids.erase(ids.begin()+randInd);
		sizeMod[i] = glm::vec2(getRandF(0.5f, 20.f), getRandF(0.5f, 20.f));
	}

	initShdr();
}

//---------------------------------------------------------------

void SNTexMosaicShuffler::initShdr()
{
	//- Position Shader ---
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
					layout (location=0) in vec4 position;\n
					layout (location=1) in vec3 normal;\n
					layout (location=2) in vec2 texCoord;\n
					layout (location=3) in vec4 color;\n
					out vec2 tex_coord;\n
					void main() {\n
						tex_coord = texCoord;
						gl_Position = position;\n
					});
	vert = "// SNTexMosaicShuffler pos tex vertex shader\n" + shdr_Header + vert;


	std::string frag = "uniform int randIds["+std::to_string(nrIds)+"];\n uniform vec2 sizeMod["+std::to_string(nrIds)+"];\n"+
			STRINGIFY(layout(location = 0) out vec4 fragColor;\n
					in vec2 tex_coord;\n
					uniform sampler2D tex;\n
					uniform ivec2 gridSize;\n
					uniform int nrIds;\n
					uniform float ySinScale;\n
					uniform float ySinFreq;\n
					uniform float ySinScale2;\n
					uniform float ySinFreq2;\n
					uniform float ySinScale3;\n
					uniform float ySinFreq3;\n
					uniform float ySinScale4;\n
					uniform float ySinFreq4;\n
					void main()\n {\n

						vec2 mod_tex = vec2(
							tex_coord.x,
							tex_coord.y * sin(ySinFreq2 * tex_coord.y) * ySinScale
								+ sin(tex_coord.y * ySinFreq) * ySinScale2);

						ivec2 id_tex_coord = ivec2(mod_tex * vec2(gridSize));
						id_tex_coord.x *= int(sin(ySinFreq3 * tex_coord.y) * ySinScale3);
						id_tex_coord.x += int(sin(tex_coord.y * ySinFreq4) * ySinScale4);

						int mId = id_tex_coord.x + id_tex_coord.y * gridSize.x;
						int mapId = min(randIds[mId], nrIds);

						//vec2 in_texCoord = tex_coord * vec2(sizeMod[mapId]);

						vec2 texCoord = tex_coord
							+ vec2(
								float( mod(mapId, gridSize.x) ) / float(gridSize.x),
								float( mapId / gridSize.x ) / float(gridSize.y));\n

						fragColor = texture(tex, texCoord);\n
					});

	frag = "// SNTexMosaicShuffler pos tex shader\n" + shdr_Header + frag;

	shufflShdr = shCol->addCheckShaderText("SNTexMosaicShuffler", vert.c_str(), frag.c_str());
}

//---------------------------------------------------------------

void SNTexMosaicShuffler::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{

	if (_tfo) _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);

	shufflShdr->begin();
	shufflShdr->setUniform1i("tex", 0);
	shufflShdr->setUniformMatrix4fv("m_pvm", cp->mvp);
	shufflShdr->setUniform1i("nrIds", nrIds);
	shufflShdr->setUniform1iv("randIds", &randIds[0], nrIds);
	shufflShdr->setUniform2i("gridSize", gridSizeX, gridSizeY);
	shufflShdr->setUniform2fv("sizeMod", glm::value_ptr(sizeMod[0]), nrIds);
	shufflShdr->setUniform1f("ySinScale", ySinScale);
	shufflShdr->setUniform1f("ySinFreq", ySinFreq);
	shufflShdr->setUniform1f("ySinScale2", ySinScale2);
	shufflShdr->setUniform1f("ySinFreq2", ySinFreq2);
	shufflShdr->setUniform1f("ySinScale3", ySinScale3);
	shufflShdr->setUniform1f("ySinFreq3", ySinFreq3);
	shufflShdr->setUniform1f("ySinScale4", ySinScale4);
	shufflShdr->setUniform1f("ySinFreq4", ySinFreq4);

	tex0->bind(0);

	quad->draw();
}

//---------------------------------------------------------------

void SNTexMosaicShuffler::update(double time, double dt)
{
}

//---------------------------------------------------------------

SNTexMosaicShuffler::~SNTexMosaicShuffler()
{
}

}
