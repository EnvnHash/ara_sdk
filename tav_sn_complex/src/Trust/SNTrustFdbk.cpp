//
// SNTrustFdbk.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTrustFdbk.h"

#define STRINGIFY(A) #A


namespace tav
{
SNTrustFdbk::SNTrustFdbk(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs,"NoLight"), vidStartOffs(20), downscale(2)
{
	VideoTextureCvActRange** vts = static_cast<VideoTextureCvActRange**>(scd->videoTextsActRange);
	vt = vts[ static_cast<unsigned short>(_sceneArgs->at("vtex0")) ];

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f,
            nullptr, 1, false);

	flipQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f,
            nullptr, 1, true);

    

	blur = new FastBlurMem(0.4f, shCol, vt->getWidth(), vt->getHeight());
	blur2 = new FastBlurMem(0.4f, shCol, vt->getWidth(), vt->getHeight());
    tMed = new GLSLTimeMedian(shCol, vt->getWidth(),	vt->getHeight(), GL_RGBA8);
    tMed2 = new GLSLTimeMedian(shCol, vt->getWidth(),	vt->getHeight(), GL_RGBA8);

    stdTex = shCol->getStdTexAlpha();
    initShdr();

	addPar("median", &median);
	addPar("flowFdbk", &flowFdbk);
	addPar("alpha", &alpha);
	addPar("clearAlpha", &clearAlpha);
	addPar("drawAlpha", &drawAlpha);

	medFbo = new FBO(shCol, vt->getWidth(), vt->getHeight(), 1);
	medFbo->clear();
	smearFbo = new FBO(shCol, vt->getWidth(), vt->getHeight(), 1);
	smearFbo->clear();
	finFbo= new FBO(shCol, vt->getWidth(), vt->getHeight(), 1);
	smearFbo->clear();
	ppFbo = new PingPongFbo(shCol, vt->getWidth(), vt->getHeight(), GL_RGBA8,
            GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT);
	ppFbo->clear();

	testTex = new TextureManager();
	testTex->loadTexture2D(*(_scd->dataPath)+"textures/landscape0.jpg");
}

//----------------------------------------------------

void SNTrustFdbk::initShdr()
{
     //- Position Shader ---

     std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

     std::string vert = STRINGIFY(layout (location=0) in vec4 position;\n
     	 	 	 	 	 	 	 layout (location=1) in vec3 normal;\n
   	 	 	 	 	 	 	 	 layout (location=2) in vec2 texCoord;\n
   	 	 	 	 	 	 	 	 layout (location=3) in vec4 color;\n
   	 	 	 	 	 	 	 	 out vec2 tex_coord;
   	 	 	 	 	 	 	 	 void main() {\n
   	 	 	 	 	 	 	 		 tex_coord = texCoord;
                                     gl_Position = position;\n
                                 });
     vert = "// SNTrustFdbk pos tex vertex shader\n" +shdr_Header +vert;


     //- Frag Shader ---

     shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
     std::string frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n

                                  uniform sampler2D back;
                                  uniform sampler2D front;
                                  in vec2 tex_coord;

                                  void main()\n {\n
                                	  vec4 backCol = texture(back, tex_coord);
                                  	  vec4 frontCol = texture(front, tex_coord);
                                  	  vec4 diff = max((backCol - frontCol) - 0.05, 0.0);
									  float mask = (diff.r + diff.g + diff.b) * 0.4 > 0.1 ? 1.0 : 0.0;
                                  	  if (mask < 0.001){
                                  		  discard;
                                  	  } else {
                                  		  fragColor = vec4(1.0);
                                  	  }
                                  });
     frag = "// SNTrustFdbk pos tex shader\n"+shdr_Header+frag;

     maskShader = shCol->addCheckShaderText("SNTrustFdbk",
                                                         vert.c_str(), frag.c_str());

     //----
     //----

     //- Frag Shader ---

     shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
     frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n
                                  uniform sampler2D fore;
                                  uniform sampler2D mask;
                                  in vec2 tex_coord;

                                  void main()\n {\n
                                  	  vec4 maskCol = abs(texture(mask, tex_coord));
                                  	  vec4 foreCol = texture(fore, tex_coord);
//                                  	  foreCol = vec4( vec3(1.0) - foreCol.rgb, foreCol.a);
                                  	  //foreCol = vec4( foreCol.g, foreCol.r, foreCol.b, foreCol.a);
                                  	  foreCol.a = maskCol.r;
                                  	  fragColor = foreCol;
                                  });
     frag = "// SNTrustFdbk pos tex shader\n"+shdr_Header+frag;

     applyMaskShader = shCol->addCheckShaderText("SNTrustFdbkApplyMask",
                                                         vert.c_str(), frag.c_str());

     //- Frag Shader ---

     shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
     frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n
                                  uniform sampler2D tex;
     	 	 	 	 	 	 	  uniform float alpha;
                                  in vec2 tex_coord;

                                  void main()\n {\n
                                  	  vec4 col = texture(tex, tex_coord);
                                  	  fragColor = col;
                                  	  fragColor.a = (col.r + col.g + col.b) * 0.33 * col.a;
                                  	  fragColor.a *= alpha;
                                  });
     frag = "// SNTrustFdbk blend black tex shader\n"+shdr_Header+frag;

     blendBlack = shCol->addCheckShaderText("SNTrustFdbkBlendBlack",
                                                         vert.c_str(), frag.c_str());
 }

//----------------------------------------------------

void SNTrustFdbk::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	//

	medFbo->bind();
	medFbo->clear();

	maskShader->begin();
	maskShader->setIdentMatrix4fv("m_pvm");
	maskShader->setUniform1i("back", 0);
	maskShader->setUniform1i("front", 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tMed->getResTexId());
	vt->bindActFrame(1);

	quad->draw(_tfo);

	medFbo->unbind();

	//

	blur->proc( medFbo->getColorImg() );

	//

	ppFbo->dst->bind();
	ppFbo->dst->clearAlpha(clearAlpha, 0.f);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	applyMaskShader->begin();
	applyMaskShader->setIdentMatrix4fv("m_pvm");
	applyMaskShader->setUniform1i("fore", 0);
	applyMaskShader->setUniform1i("mask", 1);

	vt->bindActFrame(0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, medFbo->getColorImg() );

	quad->draw(_tfo);

	ppFbo->dst->unbind();
	ppFbo->swap();

	//

	blur2->proc(ppFbo->src->getColorImg());

	//

	ppFbo->dst->bind();
	//ppFbo->dst->clearAlpha(clearAlpha"), 0.f);

	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);
	stdTex->setUniform1f("alpha", 1.f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, blur2->getResult() );

	quad->draw(_tfo);

	ppFbo->dst->unbind();
	ppFbo->swap();

	//

	finFbo->bind();
	finFbo->clear();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);
	stdTex->setUniform1f("alpha", 1.f);

	vt->bindActFrame(0);
	quad->draw(_tfo);


	blendBlack->begin();
	blendBlack->setIdentMatrix4fv("m_pvm");
	blendBlack->setUniform1i("tex", 0);
	blendBlack->setUniform1f("alpha", drawAlpha );

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ppFbo->src->getColorImg() );

	quad->draw(_tfo);


	applyMaskShader->begin();
	applyMaskShader->setIdentMatrix4fv("m_pvm");
	applyMaskShader->setUniform1i("fore", 0);
	applyMaskShader->setUniform1i("mask", 1);

	vt->bindActFrame(0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, medFbo->getColorImg() );

	quad->draw(_tfo);


	finFbo->unbind();

	//

	tMed2->setMedian(median);
	tMed2->update( finFbo->getColorImg() );

	//

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);
	stdTex->setUniform1f("alpha", alpha);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tMed2->getResTexId() );

	flipQuad->draw(_tfo);

	_shader->begin();
}

//----------------------------------------------------

void SNTrustFdbk::update(double time, double dt)
{
	vt->updateDt(time, false);
	actUplTexId = vt->loadFrameToTexture();

    // if there is a new frame copy it to an opengl array buffer
	if(actUplTexId != 0 && actUplTexId != lastTexId)
	{
		tMed->setMedian(median);
		tMed->update( actUplTexId );
  	  	lastTexId = actUplTexId;
	}
}

//----------------------------------------------------

SNTrustFdbk::~SNTrustFdbk()
{
	delete quad;
}

}
