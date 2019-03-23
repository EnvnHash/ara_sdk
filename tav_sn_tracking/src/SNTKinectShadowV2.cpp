//
// SNTKinectShadowV2.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTKinectShadowV2.h"
#define STRINGIFY(A) #A

namespace tav
{
SNTKinectShadowV2::SNTKinectShadowV2(sceneData* _scd, std::map<std::string, float>* _sceneArgs) : SceneNode(_scd, _sceneArgs,"NoLight")
{
	osc = static_cast<OSCData*>(scd->osc);
	kin = static_cast<KinectInput*>(scd->kin);
	kinRepro = static_cast<KinectReproTools*>(scd->kinRepro);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);


	//	_winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
	//	return this->onKey(key, scancode, action, mods); });

	quad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 0.f);
	quad->rotate(M_PI, 0.f, 0.f, 1.f);
	quad->rotate(M_PI, 0.f, 1.f, 0.f);

	shdr = shCol->getStdTex();

    blur = new FastBlurMem(0.f, shCol, 512, 512, GL_RGBA8);

	transMat = glm::mat4(1.f);

	transTexId = new GLint[2];
	transTexId[0] = 0;
	transTexId[1] = 1;

	initShdr();

	addPar("blurAlpha", &blurAlpha);
	addPar("blurBright", &blurBright);
	addPar("alpha", &alpha);
	addPar("offsScale", &offsScale);
}


SNTKinectShadowV2::~SNTKinectShadowV2()
{
	delete quad;
}


void SNTKinectShadowV2::initShdr()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 2 ) in vec2 texCoord;
	layout( location = 3 ) in vec4 color;
	out vec2 tex_coord;

	uniform mat4 m_pvm;

	void main() {
		tex_coord = texCoord;
		gl_Position = m_pvm * position;
	});
	vert = "// SNTKinectShadowV2 vertex shader\n" + shdr_Header + vert;


	std::string frag = STRINGIFY(layout (location = 0) out vec4 color;
	uniform float alpha;
	uniform sampler2D tex;
	in vec2 tex_coord;
	void main() {
		float texCol = texture(tex, tex_coord).r;
		if (texCol * alpha < 0.0001){
			discard;
		} else {
			color = vec4(texCol, texCol, texCol, alpha*texCol);
		}
	});
	frag = "// SNTKinectShadowV2 Shader\n" + shdr_Header + frag;

	renderShdr = shCol->addCheckShaderText("SNTKinectShadowV2", vert.c_str(), frag.c_str());
}


void SNTKinectShadowV2::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{

	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE); // wenn enabled wird die "rueckseite" des quad nicht gezeichnet...
	glDisable(GL_DEPTH_TEST);

	glm::mat4 m_vm = cp->view_matrix_mat4 * _modelMat;
	glm::mat4 m_pvm = cp->projection_matrix_mat4 * m_vm;
    glm::mat3 normalMat = glm::mat3( glm::transpose( glm::inverse( _modelMat ) ) );

	renderShdr->begin();
	renderShdr->setUniformMatrix4fv("m_pvm", &m_pvm[0][0]);
	renderShdr->setUniform1i("tex", 0);
	renderShdr->setUniform1f("alpha", alpha * osc->alpha);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, blur->getResult());
//	glBindTexture(GL_TEXTURE_2D, kinRepro->getDepthtransTexId(1));

	quad->draw();

	renderShdr->end();
}


void SNTKinectShadowV2::update(double time, double dt)
{
	blur->setAlpha(blurAlpha);
	blur->setBright(blurBright);
	blur->setOffsScale(offsScale);

	blur->proc(kinRepro->getDepthTransTexId(1));
	blur->proc(blur->getResult());
}


void SNTKinectShadowV2::onKey(int key, int scancode, int action, int mods)
{}

}
