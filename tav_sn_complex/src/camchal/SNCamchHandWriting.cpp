//
// SNCamchHandWriting.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNCamchHandWriting.h"

namespace tav
{
    SNCamchHandWriting::SNCamchHandWriting(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
    	tex0 = texs[ static_cast<GLuint>(_sceneArgs->at("tex0")) ];

        quad = scd->stdQuad;

        stdTexAlpha = shCol->getStdTexAlpha();
        stdCol = shCol->getStdCol();

        float img0Propo = texs[0]->getHeightF() / texs[0]->getWidthF();
        scalePropoImg = glm::scale(glm::mat4(1.f), glm::vec3(1.f, img0Propo, 1.f));

        scaleMask = glm::scale(glm::mat4(1.f), glm::vec3(float(_scd->screenWidth) / float(_scd->screenHeight), 1.f, 1.f));

        addPar("alpha", &alpha);
        addPar("handPos", &handPos);
    }

    SNCamchHandWriting::~SNCamchHandWriting()
    {
        delete quad;
    }

    void SNCamchHandWriting::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glEnable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        glm::mat4 pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat * scalePropoImg;

        stdTexAlpha->begin();
        stdTexAlpha->setUniform1i("tex", 0);
        stdTexAlpha->setUniform1f("alpha", alpha);
        stdTexAlpha->setUniformMatrix4fv("m_pvm", &pvm[0][0]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex0->getId() );

        quad->draw(_tfo);


        glm::mat4 maskOffs = glm::translate(glm::mat4(1.f), glm::vec3(handPos, 0.f, 0.f));
        pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat * maskOffs * scaleMask;

        stdCol->begin();
        stdCol->setUniformMatrix4fv("m_pvm", &pvm[0][0]);

        quad->draw(_tfo);
    }

    

    void SNCamchHandWriting::update(double time, double dt)
    {
    	if (time - lastUpdt  > 0.04)
    	{
//    		std::string cmd = "alpha";
//    		std::cout << "reset" << std::endl;
//    		setOscPar(&cmd, 0.f);
    	}

    	lastUpdt = time;
    }

}
