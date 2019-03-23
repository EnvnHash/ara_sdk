//
// SNCamchBorders.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNCamchBorders.h"

namespace tav
{
    SNCamchBorders::SNCamchBorders(sceneData* _scd, std::map<std::string, float>* _sceneArgs) : SceneNode(_scd, _sceneArgs)
    {
        quad = new Quad(-1.f, -1.f, 2.f, 2.f,
                        glm::vec3(0.f, 0.f, 1.f),
                        1.f, 1.f, 1.f, 1.f);

        float propo = _scd->roomDim->x / _scd->roomDim->y;

        float lineWidth = 0.2f;

        float leftBeam = ((_scd->mark->at("lcLeftBeam").x / _scd->roomDim->x) * 2.f - 1.f) * propo;
        float lcTL1 = ((_scd->mark->at("lcTL1").x / _scd->roomDim->x) * 2.f - 1.f) * propo;
        float lcTL2 = ((_scd->mark->at("lcTL2").x / _scd->roomDim->x) * 2.f - 1.f) * propo;
        float lcTL3 = ((_scd->mark->at("lcTL3").x / _scd->roomDim->x) * 2.f - 1.f) * propo;
        float lcMS = ((_scd->mark->at("lcMS").x / _scd->roomDim->x) * 2.f - 1.f) * propo;
        float lcTR1 = ((_scd->mark->at("lcTR1").x / _scd->roomDim->x) * 2.f - 1.f) * propo;
        float lcTR2 = ((_scd->mark->at("lcTR2").x / _scd->roomDim->x) * 2.f - 1.f) * propo;
        float lcTR3 = ((_scd->mark->at("lcTR3").x / _scd->roomDim->x) * 2.f - 1.f) * propo;


        borderPos.push_back( glm::vec3(leftBeam, 0.f, 0.f) );
        borderPos.push_back( glm::vec3(lcTL1, 0.f, 0.f) );
        borderPos.push_back( glm::vec3(lcTL2, 0.f, 0.f) );
        borderPos.push_back( glm::vec3(lcTL3, 0.f, 0.f) );
        borderPos.push_back( glm::vec3(lcMS, 0.f, 0.f) );
        borderPos.push_back( glm::vec3(lcTR1, 0.f, 0.f) );
        borderPos.push_back( glm::vec3(lcTR2, 0.f, 0.f) );
        borderPos.push_back( glm::vec3(lcTR3, 0.f, 0.f) );

        /*
        		GLfloat pos[] = {
        		leftBeam, 1.f, 0.f,
				leftBeam, -1.f, 0.f,
				lcTL1, 1.f, 0.f,
				lcTL1, -1.f, 0.f,
				lcTL2, 1.f, 0.f,
				lcTL2, -1.f, 0.f,
				lcTL3, 1.f, 0.f,
				lcTL3, -1.f, 0.f,
				lcMS, 1.f, 0.f,
				lcMS, -1.f, 0.f,
				lcTR1, 1.f, 0.f,
				lcTR1, -1.f, 0.f,
				lcTR2, 1.f, 0.f,
				lcTR2, -1.f, 0.f,
				lcTR3, 1.f, 0.f,
				lcTR3, -1.f, 0.f
        };

        lines = new VAO("position:3f", GL_STATIC_DRAW);
        lines->upload(POSITION, pos, 16);
        lines->setStaticColor(1.f, 1.f, 1.f, 1.f);
        */

        stdCol = shCol->getStdCol();
    }


    SNCamchBorders::~SNCamchBorders()
    {
        delete quad;
    }


    void SNCamchBorders::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glDisable(GL_DEPTH_TEST);

        glm::mat4 scale = glm::scale(glm::mat4(1.f), glm::vec3(std::sqrt(cp->actFboSize.x) * 0.00015f, 1.f, 1.f));
        glm::mat4 pv = cp->projection_matrix_mat4 * cp->view_matrix_mat4;
        glm::mat4 pvm;

        stdCol->begin();

        for (std::vector<glm::vec3>::iterator it = borderPos.begin(); it != borderPos.end(); ++it)
        {
        	pvm = pv * glm::translate(glm::mat4(1.f), (*it)) * scale;
        	stdCol->setUniformMatrix4fv("m_pvm", &pvm[0][0]);

        	quad->draw();
        }
    }

    
    void SNCamchBorders::update(double time, double dt)
    {}
}
