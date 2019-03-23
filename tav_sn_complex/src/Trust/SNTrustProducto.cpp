//
// SNTrustProducto.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTrustProducto.h"

#define STRINGIFY(A) #A

namespace tav
{
    SNTrustProducto::SNTrustProducto(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
    	texProd = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];
        texBack = texs[static_cast<GLuint>(_sceneArgs->at("tex1"))];
        texDraw = texs[static_cast<GLuint>(_sceneArgs->at("tex2"))];

        stdTexAlpha = shCol->getStdTexAlpha();
        //stdColAlpha = _shCol->getStdColAlpha();

        // get tex0
        float img0Propo = texs[0]->getHeightF() / texs[0]->getWidthF();
        float scrAsp = float(_scd->screenHeight) / float(_scd->screenWidth);
        scalePropoImg = glm::scale(glm::mat4(1.f), glm::vec3(scrAsp, 1.f, 1.f));

        addPar("alpha", &alpha); // WIEDER AENDERN!!!
        addPar("morph", &morph);
        addPar("prodScale", &prodScale);
        addPar("prodXPos", &prodXPos);
        addPar("prodZPos", &prodZPos);

        initShdr();
   }

    void SNTrustProducto::initShdr()
    {
         //- Position Shader ---

         std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

         std::string vert = STRINGIFY(layout (location=0) in vec4 position;\n
         	 	 	 	 	 	 	 layout (location=1) in vec3 normal;\n
       	 	 	 	 	 	 	 	 layout (location=2) in vec2 texCoord;\n
       	 	 	 	 	 	 	 	 layout (location=3) in vec4 color;\n
                                     uniform mat4 m_pvm;
       	 	 	 	 	 	 	 	 out vec2 tex_coord;
       	 	 	 	 	 	 	 	 void main() {\n
       	 	 	 	 	 	 	 		 tex_coord = texCoord;
                                         gl_Position = m_pvm * position;\n
                                     });
         vert = "// SNTrustProducto pos tex vertex shader\n" +shdr_Header +vert;


         //- Frag Shader ---

         shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
         std::string frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n

                                      uniform sampler2D tex;
	 	 	  	  	  	  	  	  	  uniform float morph;
         	 	 	 	 	 	 	  uniform float alpha;
                                      in vec2 tex_coord;

                                      void main()\n {\n
                                    	  vec2 polarCoord = tex_coord - 0.5;

                                      	  float dist = length(polarCoord);
                                      	  float angle = atan(polarCoord.y, polarCoord.x);
                                          float rad = (1.0 + morph) * 16.0;

										  float percent = (rad - dist) / rad;
										  float theta = percent * percent * angle * 8.0 * (1.0 - morph +1.0);
										  float s = sin(theta);
										  float c = cos(theta);
										  vec2 swirlCoord = vec2(
												  dot(polarCoord, vec2(c, -s)),
												  dot(polarCoord, vec2(s, c)));
										  swirlCoord += 0.5;

                                    	  vec4 frontCol = texture(tex, mix(swirlCoord, tex_coord, morph));
                                    	  frontCol *= pow( mix( max(0.707 - dist, 0.0), 1.0, morph), 16.0);
                                   		  fragColor = frontCol;
                                   		  //fragColor.a = alpha * min( (frontCol.r + frontCol.g + frontCol.b) * 0.333, 0.02) / 0.02;
                                      });
         frag = "// SNTrustProducto pos tex shader\n"+shdr_Header+frag;

         prodShdr = shCol->addCheckShaderText("SNTrustProducto",
                                                             vert.c_str(), frag.c_str());
     }

    void SNTrustProducto::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glEnable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        prodMat = glm::translate(glm::mat4(1.f), glm::vec3(prodXPos, prodYPos, 0.f))
        	* glm::scale(glm::mat4(1.f), glm::vec3(prodScale, prodScale, 1.f));

        // hintergrund gradient
        stdTexAlpha->begin();
        stdTexAlpha->setIdentMatrix4fv("m_pvm");
        stdTexAlpha->setUniform1i("tex", 0);
        stdTexAlpha->setUniform1f("alpha", alpha);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texBack->getId() );
        _stdQuad->draw(_tfo);


        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        // strich_zeichnung
        stdTexAlpha->setUniform1f("alpha", alpha * 0.5f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texDraw->getId() );
        _stdQuad->draw(_tfo);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


        //glm::mat4 pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat;
        glm::mat4 pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * prodMat * scalePropoImg;

        // das product
        prodShdr->begin();
        prodShdr->setUniform1i("tex", 0);
        prodShdr->setUniform1f("alpha", alpha);
        prodShdr->setUniform1f("morph", morph);
        prodShdr->setUniformMatrix4fv("m_pvm", &pvm[0][0]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texProd->getId() );

        _stdQuad->draw(_tfo);


        _shader->begin();
    }

    void SNTrustProducto::update(double time, double dt)
    {
    }

     SNTrustProducto::~SNTrustProducto()
     {
     }
}
