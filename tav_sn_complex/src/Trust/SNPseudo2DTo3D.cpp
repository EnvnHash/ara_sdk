//
// SNPseudo2DTo3D.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNPseudo2DTo3D.h"

#define STRINGIFY(A) #A

namespace tav
{
SNPseudo2DTo3D::SNPseudo2DTo3D(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs)
{
	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];

	quad = new QuadArray(40, 40, -1.f, -1.f, 2.f, 2.f, 0.f, 0.f, 0.f, 1.f);
    godRays = new GodRays(shCol, _scd->screenWidth/2, _scd->screenHeight/2);
    fbo = new FBO(shCol, _scd->screenWidth, _scd->screenHeight, GL_RGBA8,
    		GL_TEXTURE_2D, true, 2, 1, 1, GL_CLAMP_TO_EDGE, false);

	stdTexAlpha = shCol->getStdTexAlpha();

	float img0Propo = tex0->getHeightF() / tex0->getWidthF();
	scalePropoImg = glm::scale(glm::mat4(1.f), glm::vec3(1.f, img0Propo, 1.f));

	addPar("alpha", &alpha);
	addPar("xOffs", &xOffs);
	addPar("yOffs", &yOffs);
	addPar("zOffs", &zOffs);

    addPar("exp", &exp);
    addPar("weight", &weight);
    addPar("lightX", &lightX);
    addPar("lightY", &lightY);

	initShdr();
}

//----------------------------------------------------

void SNPseudo2DTo3D::initShdr()
{
	//- Position Shader ---

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(layout (location=0) in vec4 position;\n
	layout (location=1) in vec3 normal;\n
	layout (location=2) in vec2 texCoord;\n
	layout (location=3) in vec4 color;\n

	out TO_FS {
		vec2 tex_coord;
	} vertex_out;

	const float pi = 3.14159265359;
	const float quarterPi = 0.785398163397;
	uniform vec3 offsAmt;
	uniform mat4 m_pvm;

	void main() {\n
		vec2 absPos = abs(position.xy);
		float rad = sqrt( absPos.x * absPos.x + absPos.y * absPos.y );
		float a = atan( max(absPos.y, 0.0000001), max(absPos.x, 0.0000001) );

		float radScreenBorder = a > quarterPi ?
				sqrt( 1.0 + pow( 1.0 / tan(a), 2.0 ) ) :
				sqrt( 1.0 + pow( tan(a), 2.0 ) );
		float distScaleAmt = rad / radScreenBorder;

		float zOffs = cos( distScaleAmt * pi * 0.5 );

		vertex_out.tex_coord = texCoord;
		gl_Position = m_pvm * vec4(
				mix( position.xy, vec2(position.x + offsAmt.x, position.y + offsAmt.y), 1.0 - distScaleAmt ),
				position.z - max( offsAmt.z * zOffs, 0.0 ),
				1.0);\n
	});

	vert = "// SNTrustFdbk pos tex vertex shader\n" +shdr_Header +vert;


	//- Frag Shader ---

	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n
	uniform sampler2D tex;
	uniform float alpha;
	in TO_FS {
		vec2 tex_coord;
	} vertex_in;

	void main()\n {\n
		fragColor = texture(tex, vertex_in.tex_coord);
	fragColor.a = alpha;
	});
	frag = "// SNTrustFdbk pos tex shader\n"+shdr_Header+frag;

	sphrShdr = shCol->addCheckShaderText("SNPseudo2DTo3D",  vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNPseudo2DTo3D::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glm::mat4 pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat;

    godRays->setExposure( exp );
    godRays->setWeight( weight );
    godRays->setLightPosScr( lightX, lightY );

    // ----

    fbo->bind();
	sphrShdr->begin();
	sphrShdr->setUniform1i("tex", 0);
	sphrShdr->setUniform1f("alpha", 1.f);
	sphrShdr->setUniform3f("offsAmt", xOffs, yOffs, zOffs);
	sphrShdr->setUniformMatrix4fv("m_pvm", &pvm[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex0->getId() );

	quad->draw(_tfo);

	fbo->unbind();

    // ----

    godRays->bind();

    stdTexAlpha->begin();
    stdTexAlpha->setIdentMatrix4fv("m_pvm");
    stdTexAlpha->setUniform1i("tex", 0);
    stdTexAlpha->setUniform1f("alpha", 1.f);

	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbo->getColorImg());

	quad->draw(_tfo);

    godRays->unbind();

    // ----

    stdTexAlpha->begin();
    stdTexAlpha->setIdentMatrix4fv("m_pvm");
    stdTexAlpha->setUniform1i("tex", 0);
    stdTexAlpha->setUniform1f("alpha", alpha);

	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbo->getColorImg());

	quad->draw(_tfo);

    // ----

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    godRays->draw();

	_shader->begin();
}

//----------------------------------------------------

void SNPseudo2DTo3D::update(double time, double dt)
{
}

//----------------------------------------------------

SNPseudo2DTo3D::~SNPseudo2DTo3D()
{
	delete quad;
}
}
