//
// SNTestTvPatron.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestTvPatron.h"

namespace tav
{
    SNTestTvPatron::SNTestTvPatron(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs,"NoLight")
    {
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        quad = scd->stdQuad;

        //quad->rotate(-M_PI_2, 1.f, 0.f, 0.f);
        //quad->translate(0.f, -1.001f, 0.f);
        
        stdTex = shCol->getStdTex();

        patron = new TextureManager();
        patron->loadTexture2D((*scd->dataPath) + "textures/tv.jpg");
    }
    

    SNTestTvPatron::~SNTestTvPatron()
    {
        delete quad;
    }
    

    void SNTestTvPatron::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        glEnable(GL_BLEND);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

//        sendStdShaderInit(_shader);
//        useTextureUnitInd(0, patron->getId(), _shader, _tfo);

        stdTex->begin();
        stdTex->setIdentMatrix4fv("m_pvm");
        stdTex->setUniform1i("tex", 0);
        patron->bind(0);

        quad->draw();
    }
    
    
    void SNTestTvPatron::update(double time, double dt)
    {}
 }
