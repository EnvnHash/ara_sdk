//
//  SNExplodeGeo.cpp
//  tav_scene
//
//  Created by Sven Hahne on 09/03/16.
//  Copyright © 2016 Sven Hahne. All rights reserved.
//

#include "SNExplodeGeo.h"

#define STRINGIFY(A) #A

namespace tav
{
SNExplodeGeo::SNExplodeGeo(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "DirLight"), objSize(0.3f)
{
	osc = static_cast<OSCData*>(scd->osc);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	quadAr = new QuadArray(40, 40, -1.f, -1.f, 0.4f, 1.f, 1.f, 1.f, 1.f, 1.f);
	objTex = new TextureManager();
	objTex->loadTexture2D((*scd->dataPath) + "/textures/samsung/g7.png");

	explSlid = new AnimVal<float>(tav::RAMP_LIN_UP, [this]()
	{
		standby->start(0.f, 1.f, 4.0, actTime, false);
	});

	standby = new AnimVal<float>(tav::RAMP_LIN_UP, [this]()
	{	this->restartMorph();});
}

//---------------------------------------------------------------

void SNExplodeGeo::initShdr(camPar* cp)
{
	std::string nrCams = std::to_string(cp->nrCams);

	//- Position Shader ---

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position;\n layout (location=1) in vec3 normal;\n layout (location=2) in vec2 texCoord;\n layout (location=3) in vec4 color;\n out vec2 tex_coord;\n void main() {\n tex_coord = texCoord;\n gl_Position = position;\n });
	vert = "// SNExplodeGeo pos tex vertex shader\n" + shdr_Header + vert;

	shdr_Header =
			"#version 410 core\n#pragma optimize(on)\n layout(triangles, invocations="
					+ nrCams
					+ ") in;\n layout(triangle_strip, max_vertices=3) out;\n uniform mat4 view_matrix_g["
					+ nrCams + "];\n uniform mat4 projection_matrix_g[" + nrCams
					+ "];\n";

	std::string geom =
			STRINGIFY(
					in vec2 tex_coord[];\n out vec2 gs_texCo;\n out vec4 gs_Col;\n

					uniform mat4 model_matrix;\n uniform sampler3D perlNoise;\n uniform float moduAmt;\n vec3 noiseModVec;\n

					void main() { noiseModVec = texture(perlNoise, vec3(tex_coord[0], 0.0)).xyz; noiseModVec -= vec3(0.5, 0.5, 0.0); noiseModVec *= vec3(20.0, 5.0, -1.0); noiseModVec = vec3((abs(noiseModVec.xy) + 3.0) * sign(noiseModVec.xy), noiseModVec.z);
					//noiseMod.z *= -1.0;

					for (int i=0; i<gl_in.length(); i++) { gl_ViewportIndex = gl_InvocationID;\n gs_texCo = tex_coord[i];\n gs_Col = vec4(noiseModVec, 1.0);\n

					gl_Position = projection_matrix_g[gl_InvocationID] * \n view_matrix_g[gl_InvocationID] * \n model_matrix * \n ( vec4(noiseModVec, 0.0) * moduAmt + gl_in[i].gl_Position );

					EmitVertex(); } EndPrimitive(); });

	geom = "// SNExplodeGeo pos tex geom shader\n" + shdr_Header + geom;

	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 fragColor;\n in vec2 gs_texCo;\n in vec4 gs_Col;\n uniform sampler2D tex;\n

					void main()\n {\n fragColor = texture(tex, gs_texCo);\n });
	frag = "// SNExplodeGeo pos tex shader\n" + shdr_Header + frag;

	explShdr = shCol->addCheckShaderText("SNExlopdeGeoShader", vert.c_str(),
			geom.c_str(), frag.c_str());
}

//---------------------------------------------------------------

void SNExplodeGeo::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (!inited)
	{
		initShdr(cp);

		noiseTex = new Noise3DTexGen(shCol, true, 4, 256, 256, 64, 4.f, 4.f,
				16.f);

		explSlid->start(1.f, 0.f, 4.0, time, false);
		inited = true;
	}

	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);
	}

	actTime = time;

//        sendStdShaderInit(_shader);
//        useTextureUnitInd(0, scd->audioTex->getTex2D(), _shader, _tfo);
	glm::mat4 modelMat = glm::translate(glm::mat4(1.f),
			glm::vec3(0.f, 0.f, -cp->roomDimen->z * 0.5f));
	modelMat = glm::scale(modelMat,
			glm::vec3(
					float(scd->screenHeight) / float(scd->screenWidth)
							* (objTex->getWidthF() / objTex->getHeightF())
							* objSize, objSize, 1.f));
	//glm::mat3 normalMat = glm::mat3( glm::transpose( glm::inverse( modelMat ) ) );

	explShdr->begin();
	explShdr->setUniform1i("tex", 0);
	explShdr->setUniform1i("perlNoise", 1);
	explShdr->setUniform1f("moduAmt", osc->blurFboAlpha);
//        explShdr->setUniform1f("modu", explSlid->getVal());
	explShdr->setUniformMatrix4fv("model_matrix", &modelMat[0][0]);
	//explShdr->setUniformMatrix3fv("normalMatrix", &normalMat[0][0]);
	explShdr->setUniformMatrix4fv("projection_matrix_g",
			cp->multicam_projection_matrix, cp->nrCams);
	explShdr->setUniformMatrix4fv("view_matrix_g", cp->multicam_view_matrix,
			cp->nrCams);

	glActiveTexture (GL_TEXTURE0);
	objTex->bind(0);

	glActiveTexture (GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, noiseTex->getTex());

	quadAr->draw();
}

//---------------------------------------------------------------

void SNExplodeGeo::update(double time, double dt)
{
	explSlid->update(time);
	standby->update(time);
}

//---------------------------------------------------------------

void SNExplodeGeo::restartMorph()
{
	if (explSlid->getVal() >= 1.f)
	{
		explSlid->reset();
		explSlid->start(1.f, 0.f, 4.0, actTime, false);
	}
	else
	{
		explSlid->reset();
		explSlid->start(0.f, 1.f, 4.0, actTime, false);
	}
}

//---------------------------------------------------------------

SNExplodeGeo::~SNExplodeGeo()
{
}

}
