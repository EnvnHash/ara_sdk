//
// SNTrustScatter.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTrustScatter.h"

#define STRINGIFY(A) #A


namespace tav
{
SNTrustScatter::SNTrustScatter(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs,"NoLight")
{
	VideoTextureCvActRange** vts = static_cast<VideoTextureCvActRange**>(scd->videoTextsActRange);
	vt = vts[ static_cast<unsigned short>(_sceneArgs->at("vtex0")) ];

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f,
            nullptr, 1, true);

	quadNoFlip = new Quad(-1.f, -1.f, 2.f, 2.f,
				glm::vec3(0.f, 0.f, 1.f),
				0.f, 0.f, 0.f, 1.f,
	            nullptr, 1, false);

	lightCircle = new Circle(20, 0.1f, 0.f);
    lightPos = glm::vec3(0.8f, 0.4f, 0.f);

    addPar("alpha", &alpha);
	addPar("density", &density);
	addPar("exposure", &exposure);
	addPar("voluNetAlpha", &voluNetAlpha);
	addPar("lightX", &lightX);
	addPar("lightY", &lightY);
	addPar("grAlpha", &grAlpha);

    godRays = new GodRays(shCol, _scd->screenWidth/2, _scd->screenHeight/2);

	godRaysFbo = new FBO(shCol, _scd->screenWidth/2, _scd->screenHeight/2,
						GL_RGBA8, GL_TEXTURE_2D, false, 1, 0, 1, GL_CLAMP_TO_EDGE, false);
	maskFbo = new FBO(shCol, _scd->screenWidth/2, _scd->screenHeight/2,
						GL_RGBA8, GL_TEXTURE_2D, false, 1, 0, 1, GL_CLAMP_TO_EDGE, false);
    tMed = new GLSLTimeMedian(shCol, vt->getWidth(),	vt->getHeight(), GL_RGBA8);
    optFlow = new GLSLOpticalFlow(shCol, vt->getWidth(),	vt->getHeight());
	blur = new FastBlurMem(0.48f, shCol,  _scd->screenWidth/2, _scd->screenHeight/2, GL_RGBA8);

	stdColShdr = shCol->getStdCol();
    stdTex = shCol->getStdTexAlpha();
	initShdr();
    initGodRayShdr();
}

//----------------------------------------------------

void SNTrustScatter::initShdr()
{
    //- Position Shader ---

    std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

    std::string vert = STRINGIFY(layout (location=0) in vec4 position;\n
    	 	 	 	 	 	 	 layout (location=1) in vec3 normal;\n
  	 	 	 	 	 	 	 	 layout (location=2) in vec2 texCoord;\n
  	 	 	 	 	 	 	 	 layout (location=3) in vec4 color;\n
  	 	 	 	 	 	 	 	 out vec2 tex_coord;
                                 uniform mat4 m_pvm;
  	 	 	 	 	 	 	 	 void main() {\n
  	 	 	 	 	 	 	 		 tex_coord = texCoord;
                                    gl_Position = m_pvm * position;\n
                                });
    vert = "// SNTrustFdbk pos tex vertex shader\n" +shdr_Header +vert;


    //- Frag Shader ---

    shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
    std::string frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n
                                 uniform sampler2D mask;
                                 in vec2 tex_coord;

                                 void main()\n {\n
                                 	 vec4 maskCol = texture(mask, tex_coord);
                                 	 float mask = abs(maskCol.r) + abs(maskCol.g);
                                 	 mask = min(sqrt(mask * 1.8), 1.0);
                                 	 fragColor = vec4(mask);
                                 });
    frag = "// SNTrustFdbk pos tex shader\n"+shdr_Header+frag;

    maskShader = shCol->addCheckShaderText("SNTrustScatterMask",
                                                        vert.c_str(), frag.c_str());

    //- Frag Shader ---

    shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
    frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n
                                 uniform sampler2D fore;
                                 uniform sampler2D mask;
                                 in vec2 tex_coord;
                                 bool first = true;\n
								 float pi = 3.14159265358979323846;\n

								 vec3 computeColor(float fx, float fy)\n
								 {
									 // relative lengths of color transitions:
									 // these are chosen based on perceptual similarity
									 // (e.g. one can distinguish more shades between red and yellow
									 //  than between yellow and green)
									 const int RY = 15;\n
									 const int YG = 6;\n
									 const int GC = 4;\n
									 const int CB = 11;\n
									 const int BM = 13;\n
									 const int MR = 6;\n
									 const int NCOLS = RY + YG + GC + CB + BM + MR;\n
									 vec3 colorWheel[NCOLS];\n

									 if (first)\n
									 {\n
										 int k = 0;\n

										 for (int i = 0; i < RY; ++i, ++k)\n
											 colorWheel[k] = vec3(255.0, float(255 * i) / RY, 0);\n

										 for (int i = 0; i < YG; ++i, ++k)\n
											 colorWheel[k] = vec3(float(255 - 255 * i) / YG, 255.0, 0);\n

										 for (int i = 0; i < GC; ++i, ++k)
											 colorWheel[k] = vec3(0, 255.0, float(255 * i) / GC);\n

										 for (int i = 0; i < CB; ++i, ++k)
											 colorWheel[k] = vec3(0, float(255 - 255 * i) / CB, 255.0);\n

										 for (int i = 0; i < BM; ++i, ++k)\n
											 colorWheel[k] = vec3(float(255 * i) / BM, 0, 255.0);\n

										 for (int i = 0; i < MR; ++i, ++k)\n
											 colorWheel[k] = vec3(255.0, 0, float(255 - 255 * i) / MR);\n

										 first = false;\n
									 }\n

									 float rad = sqrt(fx * fx + fy * fy);\n
									 float a = atan(-fy, -fx) / pi;\n
									 float fk = (a + 1.0) / 2.0 * float(NCOLS - 1);\n
									 int k0 = int(fk);\n
									 int k1 = (k0 + 1) % NCOLS;\n
									 float f = fk - float(k0);\n

									 vec3 pix;\n

									 for (int b = 0; b < 3; b++)\n
									 {
										 float col0 = colorWheel[k0][b] / 255.0;\n
										 float col1 = colorWheel[k1][b] / 255.0;\n
										 float col = (1.0 - f) * col0 + f * col1;\n

										 if (rad <= 1.0) {\n
											 col = 1.0 - rad * (1.0 - col);\n // increase saturation with radius
										  } else {\n
											 col *= .75;\n // out of range
										  }
										 pix[2 - b] = col;\n
									 }\n

									 return pix;
								 }

                                 void main()\n {\n
                                 	 vec4 maskCol = texture(mask, tex_coord);
                                 	 vec4 foreCol = texture(fore, tex_coord);
                                 	 foreCol = vec4(computeColor(foreCol.x, foreCol.y), 1.0);
                                 	 foreCol.a = maskCol.r;
                                 	 fragColor = foreCol;
                                 });
    frag = "// SNTrustFdbk pos tex shader\n"+shdr_Header+frag;

    applyMaskShader = shCol->addCheckShaderText("SNTrustScatterApplyMask",
                                                        vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNTrustScatter::initGodRayShdr()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string vert = STRINGIFY(
	layout(location = 0) in vec4 position;
	layout(location = 1) in vec4 normal;
	layout(location = 2) in vec2 texCoord;

	uniform mat4 m_pvm;
	uniform vec2 lightPositionOnScreen;
	uniform float density;

	const int NUM_SAMPLES = 80;

	out toFrag
	{
		vec2 texCoord;
		vec2 deltaTextCoord;
	} vertex_out;

	void main() {
		vertex_out.deltaTextCoord = vec2(texCoord - lightPositionOnScreen.xy );
		vertex_out.deltaTextCoord *= 1.0 /  float(NUM_SAMPLES) * density;

		vertex_out.texCoord = texCoord;
		gl_Position = m_pvm * position;
	});

	vert = "// SNIconParticles GodRayShdr Shader\n" + shdr_Header + vert;

	

	std::string frag = STRINGIFY(
	layout (location = 0) out vec4 color;

	uniform sampler2D firstPass;
	uniform float exposure;
	uniform float decay;
	uniform float weight;
	uniform float alpha;

	const int NUM_SAMPLES = 80;

	in toFrag {
		vec2 texCoord;
		vec2 deltaTextCoord;
	} vertex_in;

	void main()
	{
		vec2 textCoo = vertex_in.texCoord;
		float illuminationDecay = 1.0;
		vec4 outCol = vec4(0.0);

		for(int i=0; i < NUM_SAMPLES ; i++)
		{
			 textCoo -= vertex_in.deltaTextCoord;
			 vec4 sampleCol = texture(firstPass, textCoo );
			 sampleCol *= illuminationDecay * weight;
			 outCol += sampleCol;
			 illuminationDecay *= decay;
		 }

		outCol *= exposure;

		float bright = (outCol.r + outCol.g + outCol.b) * 0.33333;
		outCol.a *= bright;
		outCol.a *= alpha;

		if (outCol.a < 0.02) discard;

		color = outCol;
	});

	frag = "// SNIconParticles GodRayShdr Shader\n" + shdr_Header + frag;

	godRayShdr =shCol->addCheckShaderText("trustScatterGodRayShdr", vert.c_str(), frag.c_str());;
}

//----------------------------------------------------

void SNTrustScatter::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glm::mat4 circMvp = glm::translate(cp->projection_matrix_mat4 * cp->view_matrix_mat4, lightPos);
	glm::vec2 lightPosScreen = glm::vec2(lightX, lightY);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	stdTex->begin();
//	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniformMatrix4fv("m_pvm", vt->getTransMatPtr());

	stdTex->setUniform1i("tex", 0);
	stdTex->setUniform1f("alpha", alpha);
	vt->bindActFrame(0);
	glActiveTexture(GL_TEXTURE0);
	quad->draw(_tfo);

	//--- generate mask --

	// draw net to fbo
	maskFbo->bind();
	maskFbo->clearAlpha(0.5f, 0.f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	maskShader->begin();
//	maskShader->setIdentMatrix4fv("m_pvm");
	maskShader->setUniformMatrix4fv("m_pvm", vt->getTransMatFlipHPtr());
	maskShader->setUniform1i("mask", 1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, optFlow->getResTexId() );

	quadNoFlip->draw(_tfo);

	maskFbo->unbind();

	//--- blur mask --

	blur->proc(maskFbo->getColorImg());

	//--- apply mask --
	//--- prepare godray Fbo--

	// draw net to fbo
	godRaysFbo->bind();
	godRaysFbo->clear();
//	godRays->bind();

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	applyMaskShader->begin();
	applyMaskShader->setIdentMatrix4fv("m_pvm");
	applyMaskShader->setUniform1i("fore", 0);
	applyMaskShader->setUniform1i("mask", 1);

	//vt->bindActFrame(0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, optFlow->getResTexId() );

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, blur->getResult() );

	quadNoFlip->draw(_tfo);

	godRaysFbo->unbind();
	//godRays->unbind();

	//--- render go rays --

	// render result with godray shader
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    godRays->setExposure( exposure );
    godRays->setDensity( density );
    godRays->setDecay( decay );
    godRays->setWeight( 5.65f );
    godRays->setLightPosScr( lightPosScreen.x, lightPosScreen.y );
   // godRays->setAlpha( grAlpha );

//    godRays->draw();

	godRayShdr->begin();
	godRayShdr->setIdentMatrix4fv("m_pvm");
	godRayShdr->setUniform1i("firstPass", 0);

	godRayShdr->setUniform1f("exposure", exposure);
	godRayShdr->setUniform1f("decay", 0.9999f);
	godRayShdr->setUniform1f("density", density);
	godRayShdr->setUniform1f("alpha", alpha * voluNetAlpha);
	godRayShdr->setUniform1f("weight", 5.65f);
	godRayShdr->setUniform2f("lightPositionOnScreen", lightPosScreen.x, lightPosScreen.y);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, godRaysFbo->getColorImg());

	quad->draw();

	godRayShdr->end();

	/*
	//--- debug--

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);
	stdTex->setUniform1f("alpha", 1.f);
	//vt->bindActFrame(0);
	glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, optFlow->getResTexId() );
//	glBindTexture(GL_TEXTURE_2D, godRaysFbo->getColorImg());
	glBindTexture(GL_TEXTURE_2D, maskFbo->getColorImg());
	//glBindTexture(GL_TEXTURE_2D, blur->getResult() );

	quad->draw(_tfo);
	 */
}

//----------------------------------------------------

void SNTrustScatter::update(double time, double dt)
{
	vt->updateDt(time, false);
	actUplTexId = vt->loadFrameToTexture();

    // if there is a new frame copy it to an opengl array buffer
	if(actUplTexId != 0 && actUplTexId != lastTexId)
	{
		tMed->setMedian(1.f);
		tMed->update( actUplTexId );

		//optFlow->setPVM( vt->getTransMatPtr() );
		optFlow->update(tMed->getResTexId(), tMed->getLastResId());

  	  	lastTexId = actUplTexId;
	}
}

//----------------------------------------------------

SNTrustScatter::~SNTrustScatter()
{
	delete quad;
}

}
