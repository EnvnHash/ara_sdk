//
// SNTestSphere.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestSphere.h"

namespace tav
{
    SNTestSphere::SNTestSphere(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
    	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
    	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];

        quad = new Quad(-0.5f, -0.5f, 1.f, 1.f,
                        glm::vec3(0.f, 0.f, 1.f),
                        1.f, 0.f, 0.f, 1.f);    // color will be replace when rendering with blending on
        
        sphere = new Sphere(1.f, 64);
        cubeElem = new CubeElem(0.5f, 0.5f, 0.5f);
    }
    
    SNTestSphere::~SNTestSphere()
    {
        delete quad;
        delete sphere;
    }
    
    void SNTestSphere::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        sendStdShaderInit(_shader);
        useTextureUnitInd(0, tex0->getId(), _shader, _tfo);
        //sphere->draw();
        cubeElem->draw(_tfo);
    }
    
    void SNTestSphere::update(double time, double dt)
    {}
    
}
