//
// SNCamchCircleOrbit.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNCamchCircleOrbit.h"

#define STRINGIFY(A) #A

using namespace glm;

namespace tav
{
SNCamchCircleOrbit::SNCamchCircleOrbit(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs,"LitSphere"), inited(false)
{
	osc = static_cast<OSCData*>(scd->osc);
	pa = (PAudio*) scd->pa;

	// setup chanCols
	getChanCols(&chanCols);

	nrInst = 40;
	nrVar = 5;

	yAmp = 0.4f;
	circBaseSize = 1.5f;
	rotSpeed = 0.07f;
	depthScale = 20.0f;
	scaleAmt = 2.f;

	addPar("zDepth", &zDepth);
	addPar("zSpeed", &zSpeed);
	addPar("overBright", &overBright);
	addPar("backAlpha", &backAlpha);
	addPar("alpha", &alpha);

    propo = _scd->roomDim->x / _scd->roomDim->y;

    litTex = new TextureManager();
    litTex->loadTexture2D(*scd->dataPath+"/textures/litspheres/ikin_logo_lit.jpeg");

//    cubeTex = new TextureManager();
//    cubeTex->loadTextureCube( ((*scd->dataPath)+"textures/skyboxsun5deg2.png").c_str() );

    circs = new Circle*[nrVar];
    rotDir = new float[nrVar];

    for (int i=0;i<nrVar;i++)
    {
    	float fInd = static_cast<float>(i+1) / static_cast<float>(nrVar);
    	std::vector<coordType> instAttribs;
    	circs[i] = new Circle(
    			static_cast<int>(getRandF(60, 80)),
				getRandF(1.f +fInd, 2.f),
    			getRandF(0.8f +fInd, 1.8f),
    			float(M_PI) * getRandF(fInd * 1.5f, fInd * 2.f),
    			1.f, 1.f, 1.f, 1.f,
    			&instAttribs, nrInst);

    	rotDir[i] = (i % 2 == 0 ? 1.f : -1.f) * getRandF(0.7f, 1.f);
    }

    int fboWidth = std::min(_scd->screenWidth, 2048);
    int fboHeight = int(float(_scd->screenHeight) * float(fboWidth) / float(_scd->screenWidth));

    circFbo = new FBO(shCol, fboWidth, fboHeight,
	    	GL_RGBA8, GL_TEXTURE_2D, true, 1, 0, 1, GL_CLAMP_TO_EDGE, false);

    depthBlurFbo = new FBO(shCol, fboWidth, fboHeight,
	    	GL_RGBA8, GL_TEXTURE_2D, false, 1, 0, 1, GL_CLAMP_TO_EDGE, false);

    quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	blur = new FastBlurMem(0.48f, shCol, fboWidth, fboHeight, GL_RGBA8);

    stdTexAlpha = shCol->getStdTexAlpha();
    initDepthShdr();
    initBlurOverlayShdr();
}

//-----------------------------------------------------------------

SNCamchCircleOrbit::~SNCamchCircleOrbit()
{}

//-----------------------------------------------------------------

void SNCamchCircleOrbit::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	float varOffs;

	if(!inited)
	{
		initShdr(_tfo);
		inited = true;
	}



	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// render circs to fbo
	circFbo->bind();
	circFbo->clearAlpha(0.3f, 1.f);

	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);	// muss hier stehen
	glEnable(GL_CULL_FACE);

	glm::mat4 mvp = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat
			 * glm::scale(glm::mat4(1.f), glm::vec3(propo, 1.f, 1.f));
    glm::mat3 m_normal = glm::mat3( glm::transpose( glm::inverse( mvp ) ) );

	recShdr->begin();
	recShdr->setUniformMatrix4fv("m_pvm", &mvp[0][0]);
	recShdr->setUniformMatrix3fv("m_normal", &m_normal[0][0]);
	recShdr->setUniform1f("nrInst", float(nrInst));
	recShdr->setUniform1f("zDepth",	zDepth);
	recShdr->setUniform1f("time", time * zSpeed );

	recShdr->setUniform1i("litTex", 0);
	litTex->bind(0);

	for (int i=0;i<nrVar;i++)
	{
		varOffs = 0.5f / float(nrInst) / float(nrVar);

		recShdr->setUniform1f("rotDir", rotDir[i]);
		recShdr->setUniform1f("zOffs", varOffs * static_cast<float>(i));
		recShdr->setUniform4fv("col", &chanCols[i % 3][0]);
		circs[i]->drawInstanced(nrInst, nullptr, 1.f);
	}

	circFbo->clearAlpha( backAlpha, 1.f); // soften
	circFbo->unbind();

	//---------------------------------

	glDisable(GL_DEPTH_TEST);

	depthBlurFbo->bind();
	depthBlurFbo->clear();

	depthShdr->begin();
	depthShdr->setIdentMatrix4fv("m_pvm");
	depthShdr->setUniform1i("tex", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, circFbo->getDepthImg());
	quad->draw();

	depthBlurFbo->unbind();

	//---------------------------------

	blur->proc(depthBlurFbo->getColorImg());

	//---------------------------------

	// draw circle fbo
	stdTexAlpha->begin();
	stdTexAlpha->setIdentMatrix4fv("m_pvm");
	stdTexAlpha->setUniform1i("tex", 0);
	stdTexAlpha->setUniform1f("alpha", alpha);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, circFbo->getColorImg());

	quad->draw();

	//---------------------------------

	// draw overlay
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	blurOvr->begin();
	blurOvr->setIdentMatrix4fv("m_pvm");
	blurOvr->setUniform1i("tex", 0);
	blurOvr->setUniform1f("bright", overBright);
	blurOvr->setUniform1f("alpha", alpha);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, blur->getResult());

	quad->draw();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//-----------------------------------------------------------------

void SNCamchCircleOrbit::initShdr(TFO* _tfo)
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
			layout (location=0) in vec4 position;\n
			layout (location=1) in vec3 normal;\n
			layout (location=2) in vec2 texCoord;\n
			layout (location=3) in vec4 color;\n

			uniform float nrInst;\n
			uniform float time;\n
			uniform float zDepth;\n
			uniform float rotDir;\n
			uniform float zOffs;\n
			uniform mat4 m_pvm;\n
			uniform mat3 m_normal;\n

			float rotZ;\n
			float offsX;\n
			float offsY;\n
			float offsZ;\n
			float scale;\n
			float pi = 3.1415926535897932384626433832795;\n

			float angle;\n

			vec4 column0;\n
			vec4 column1;\n
			vec4 column2;\n
			vec4 column3;\n

			vec4 modPos;\n

			mat4 scale_matrix;
			mat4 trans_matrix;

			out to_fs {
				vec4 pos;
				vec2 texCoord;
				vec3 normal;
			} vertex_out;

			void main(void)
			{
				offsX = 0.0;
				offsY = 0.0;
				offsZ = 1.0 - mod(float(gl_InstanceID) / nrInst + time + zOffs, 1.0);

				angle = atan(position.y, position.x) / (2.0 * pi);

				// angle offset per instance
				rotZ = (float(gl_InstanceID) / nrInst) * pi * 13.0;
				// angle offset per time
				rotZ += mod(time * rotDir, 1.0) * pi * 2.0;


				column0 = vec4(cos(rotZ), sin(rotZ), 0.0, 0.0);
				column1 = vec4(-sin(rotZ), cos(rotZ), 0.0, 0.0);
				column2 = vec4(0.0, 0.0, 1.0, 0.0);
				column3 = vec4(offsX, offsY, offsZ, 1.0);
				trans_matrix = mat4(column0, column1, column2, column3);

				modPos = trans_matrix * position;

				vertex_out.texCoord = texCoord;
				vertex_out.normal = m_normal * normal;
				vertex_out.pos = modPos;

				modPos.z *= -zDepth;

				gl_Position = m_pvm * modPos;
//				gl_Position = m_pvm * trans_matrix * scale_matrix * position;
			});
	vert = "// SNCamchCircleOrbit record shader\n" +shdr_Header +vert;


	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string frag = STRINGIFY(layout(location = 0) out vec4 color;

	in to_fs {
		vec4 pos;	// z von 0 | -1
		vec2 texCoord;
		vec3 normal;
	} vertex_in;

	uniform sampler2D litTex;\n
	uniform vec4 col;\n

	void main() {

		vec3 L = normalize(vec3(1.0, 0.3, 0.0) - vertex_in.pos.xyz);\n
		vec3 R = normalize(-reflect(L, vertex_in.normal.xyz));\n

		vec4 litColor = texture(litTex, vec2(vertex_in.normal) * 0.5 + vec2(0.5));
		//color = vec4(1.0, 1.0, 1.0, 0.3);
		color = mix(litColor, col, 0.2) * 1.15;
		color.a *= 1.0 - vertex_in.pos.z;
	});

	frag = "// SNCamchCircleOrbit record shader\n"+shdr_Header+frag;

	recShdr = shCol->addCheckShaderText("camchCircleOrbit", vert.c_str(), frag.c_str());
}

//-----------------------------------------------------------------

void SNCamchCircleOrbit::initDepthShdr()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
			layout (location=0) in vec4 position;\n
			layout (location=1) in vec3 normal;\n
			layout (location=2) in vec2 texCoord;\n
			layout (location=3) in vec4 color;\n

			uniform mat4 m_pvm;

			out to_fs {
				vec4 pos;
				vec2 texCoord;
				vec3 normal;
			} vertex_out;

			void main(void)
			{
				vertex_out.texCoord = texCoord;
				vertex_out.normal =  normal;
				vertex_out.pos = position;
				gl_Position = m_pvm * position;
			});
	vert = "// SNCamchCircleOrbit depth shader\n" +shdr_Header +vert;


	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string frag = STRINGIFY(layout(location = 0) out vec4 color;

	in to_fs {
		vec4 pos;	// z von 0 | -1
		vec2 texCoord;
		vec3 normal;
	} vertex_in;

	uniform sampler2D tex;\n

	void main() {
		vec4 getDepth = texture(tex, vertex_in.texCoord);
		color = vec4(getDepth.r);
	});

	frag = "// SNCamchCircleOrbit record shader\n"+shdr_Header+frag;

	depthShdr = shCol->addCheckShaderText("camchCircleOrbiDepth", vert.c_str(), frag.c_str());
}

//-----------------------------------------------------------------

void SNCamchCircleOrbit::initBlurOverlayShdr()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
			layout (location=0) in vec4 position;\n
			layout (location=1) in vec3 normal;\n
			layout (location=2) in vec2 texCoord;\n
			layout (location=3) in vec4 color;\n

			uniform mat4 m_pvm;

			out to_fs {
				vec4 pos;
				vec2 texCoord;
				vec3 normal;
			} vertex_out;

			void main(void)
			{
				vertex_out.texCoord = texCoord;
				vertex_out.normal =  normal;
				vertex_out.pos = position;
				gl_Position = m_pvm * position;
			});
	vert = "// SNCamchCircleOrbit depth shader\n" +shdr_Header +vert;


	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string frag = STRINGIFY(layout(location = 0) out vec4 color;
	in to_fs {
		vec4 pos;	// z von 0 | -1
		vec2 texCoord;
		vec3 normal;
	} vertex_in;
	uniform sampler2D tex;\n
	uniform float bright;
	uniform float alpha;

	void main() {
		vec4 getCol = texture(tex, vertex_in.texCoord);
		color = vec4(getCol.rgb, (getCol.r + getCol.g + getCol.b) * 0.33 * bright);
		color.a *= alpha;
	});

	frag = "// SNCamchCircleOrbit record shader\n"+shdr_Header+frag;

	blurOvr = shCol->addCheckShaderText("camchCircleOrbOver", vert.c_str(), frag.c_str());
}

//-----------------------------------------------------------------

void SNCamchCircleOrbit::update(double time, double dt)
{
//	scd->audioTex->mergeTo2D( pa->waveData.blockCounter);
//	scd->audioTex->update(pa->getPll(), pa->waveData.blockCounter);
}

}
