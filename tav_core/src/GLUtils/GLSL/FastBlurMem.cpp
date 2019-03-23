//
//  FastBlurMem.cpp
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "GLUtils/GLSL/FastBlurMem.h"

#define STRINGIFY(A) #A

namespace tav
{
FastBlurMem::FastBlurMem(float _alpha, ShaderCollector* _shCol, int _blurW,
		int _blurH, GLenum _type, bool _rot180, blurKernelSize _kSize) :
		blurW(_blurW), blurH(_blurH), alpha(_alpha), shCol(_shCol), bright(1.f),
		rot180(_rot180), kSize(_kSize), nrRandOffs(40)
{

	fboQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);
	initShader();

	fWidth = static_cast<float>(blurW);
	fHeight = static_cast<float>(blurH);

	actKernelSize = kSize == KERNEL_3 ? 3 : 5;

	blurOffs = new GLfloat[actKernelSize];
	for (unsigned int i=0; i<actKernelSize; i++)
		blurOffs[i] = float(i) / fWidth;

	blurOffsScale = new GLfloat[actKernelSize];

	pp = new PingPongFbo(shCol, blurW, blurH, _type, GL_TEXTURE_2D, false, 1, 1,
			1, GL_CLAMP_TO_EDGE, false);
	pp->setMagFilter(GL_LINEAR);
	pp->setMinFilter(GL_LINEAR);
	pp->clear();

	firstPassFbo = new FBO(shCol, blurW, blurH, _type, GL_TEXTURE_2D, false, 1,
			1, 1, GL_CLAMP_TO_EDGE, false);
	firstPassFbo->clear();
	firstPassFbo->setMagFilter(GL_LINEAR);
	firstPassFbo->setMinFilter(GL_LINEAR);

}

//----------------------------------------------------

FastBlurMem::~FastBlurMem()
{
	delete fboQuad;
	pp->cleanUp();
	delete pp;
	linearH->remove();
	delete linearH;
	linearV->remove();
	delete linearV;

	delete[] blurOffs;
	delete[] blurOffsScale;
}

//----------------------------------------------------

void FastBlurMem::proc(GLint texIn, unsigned int itNr)
{
	glEnable(GL_BLEND);

	for (unsigned int i=0; i<actKernelSize; i++)
		blurOffsScale[i] = blurOffs[i] * offsScale;

	firstPassFbo->bind();
	firstPassFbo->clearAlpha(alpha, 0.f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	// linear vertical blur
	linearV->begin();
	linearV->setUniform1i("image", 0);
	linearV->setUniform1f("width", fWidth);
	linearV->setUniform1f("height", fHeight);
	linearV->setUniform1fv("offset", &blurOffsScale[0], actKernelSize);
	linearV->setUniform1f("rot180", rot180 ? -1.f : 1.f);
	linearV->setIdentMatrix4fv("m_pvm");

//        if (pvm_ptr == 0)
//    		linearV->setIdentMatrix4fv("m_pvm");
//    	else
//    		linearV->setUniformMatrix4fv("m_pvm", pvm_ptr);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texIn);

	fboQuad->draw();
	linearV->end();

	firstPassFbo->unbind();

	// linear horizontal blur
	pp->dst->bind();
	pp->dst->clear();

	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
	//  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	linearH->begin();
	linearH->setUniform1i("image", 0);
	linearH->setUniform1f("bright", bright);
	linearH->setUniform1f("width", fWidth);
	linearH->setUniform1f("height", fHeight);
	linearH->setUniform1fv("offset", &blurOffsScale[0], actKernelSize);
	linearH->setIdentMatrix4fv("m_pvm");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, firstPassFbo->getColorImg());
	fboQuad->draw();
	linearH->end();

	pp->dst->unbind();

	pp->swap();
}

//----------------------------------------------------

void FastBlurMem::initShader()
{
	std::string vert = STRINGIFY(
		layout(location = 0) in vec4 position;
		layout(location = 2) in vec2 texCoord;
		out vec2 tex_coord;
		void main() {
			tex_coord = texCoord;
			gl_Position = vec4(position.xy, 0.0, 1.0);
		});
	vert = "// FastBlur vertex shader\n" + shCol->getShaderHeader() + vert;

	std::string frag = STRINGIFY(
		uniform sampler2D image;\n
		uniform float width;\n
		uniform float height;\n
		uniform float rot180;\n
		\n
		vec2 flipTexCoord;\n
		vec4 col;\n
		\n
		in vec2 tex_coord;\n
		out vec4 FragmentColor;\n
	);

	if (kSize == KERNEL_3){
		frag += "uniform float offset[3];\n"
				"uniform float weight[3] = float[] (0.2270270270, 0.3162162162, 0.0702702703);\n";
	} else {
		frag += "uniform float offset[5];\n"
				"uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);\n";
	}

	frag += STRINGIFY(
		void main(){
			flipTexCoord = vec2(tex_coord.x * rot180, tex_coord.y * rot180);
			col = texture(image, flipTexCoord);
			FragmentColor = col * weight[0];
	);

	if (kSize == KERNEL_3)
		frag += "for (int i=1; i<3; i++) {\n";
	else if (kSize == KERNEL_5)
		frag += "for (int i=1; i<5; i++) {\n";

	frag += STRINGIFY(
				FragmentColor += texture(image, flipTexCoord + vec2(0.0, offset[i])) * weight[i];\n
				FragmentColor += texture(image, flipTexCoord - vec2(0.0, offset[i])) * weight[i];\n
			}
		});

	frag = "// FastBlurMem Vertical fragment shader\n" + shCol->getShaderHeader() + frag;

	linearV = shCol->addCheckShaderText("FastBlurMemVShader", vert.c_str(), frag.c_str());

	//----------------------------------------------------

	frag = STRINGIFY(
		uniform sampler2D image;\n
		uniform float width;\n
		uniform float height;\n
		uniform float bright;\n
		\n
		in vec2 tex_coord;\n
		out vec4 FragmentColor;\n
	);

	if (kSize == KERNEL_3){
		frag += "uniform float offset[3];\n"
				"uniform float weight[3] = float[] (0.2270270270, 0.3162162162, 0.0702702703 );\n";
	} else {
		frag += "uniform float offset[5];\n"
				"uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);\n";
	}

	frag += STRINGIFY(
		void main(void){\n
			FragmentColor = texture( image, tex_coord ) * weight[0];\n
	);

	if (kSize == KERNEL_3)
		frag += "for (int i=1; i<3; i++) {\n";
	else if (kSize == KERNEL_5)
		frag += "for (int i=1; i<5; i++) {\n";

	frag += STRINGIFY(
				FragmentColor += texture( image, tex_coord + vec2(offset[i], 0.0) ) * weight[i];\n
				FragmentColor += texture( image, tex_coord - vec2(offset[i], 0.0) ) * weight[i];\n
			}\n

			FragmentColor = FragmentColor * bright;
		});

	frag = "// FastBlurMem horizontal fragment shader\n" + shCol->getShaderHeader() + frag;

	linearH = shCol->addCheckShaderText("FastBlurMemHShader", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

GLint FastBlurMem::getResult()
{
	return pp->getSrcTexId();
}

//----------------------------------------------------

GLint FastBlurMem::getLastResult()
{
	return pp->getDstTexId();
}

//----------------------------------------------------

void FastBlurMem::setAlpha(float _alpha)
{
	alpha = _alpha;
}

//----------------------------------------------------

void FastBlurMem::setOffsScale(float _offsScale)
{
	offsScale = _offsScale;
}

//----------------------------------------------------

void FastBlurMem::setPVM(GLfloat* _pvm_ptr)
{
	pvm_ptr = _pvm_ptr;
}

//----------------------------------------------------

void FastBlurMem::setBright(float _bright)
{
	bright = _bright;
}
}
