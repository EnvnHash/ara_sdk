//
// SNTestTerrainTessN.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestTerrainTessN.h"

namespace tav
{
    SNTestTerrainTessN::SNTestTerrainTessN(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        addPar("heightScale", &heightScale);
        addPar("animSpeed", &animSpeed);
        addPar("skyTop", &skyTop);
        addPar("skyHeight", &skyHeight);
        addPar("ty", &ty);
        addPar("ry", &ry);

    	terrain = new TessTerrain(shCol, 2);
    }


    SNTestTerrainTessN::~SNTestTerrainTessN()
    {
        delete terrain;
    }


    void SNTestTerrainTessN::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glClearColor( 0.7f, 0.8f, 1.0f, 1.0f);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        terrain->setHeight( heightScale );
        terrain->setAnimSpeed( animSpeed );
        terrain->setSkyTop( skyTop );
        terrain->setSkyHeight( skyHeight );


        glm::mat4 trans = glm::translate(glm::vec3(0.f, ty, 0.f)) * glm::rotate(float(M_PI) * ry, glm::vec3(0.f, 1.f, 0.f));
        terrain->draw(time, dt, cp->projection_matrix_mat4, cp->view_matrix_mat4,
        		trans, cp->actFboSize, _tfo);
    }


    void SNTestTerrainTessN::update(double time, double dt)
    {}
}
