//
// SNTestQuadTexture.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestQuadTexture.h"

namespace tav
{
    SNTestQuadTexture::SNTestQuadTexture(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
    	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];

        quad = scd->stdQuad;
    }
    
    //---------------------------------------------------------------

    SNTestQuadTexture::~SNTestQuadTexture()
    {
    }

    //---------------------------------------------------------------

    void SNTestQuadTexture::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        sendStdShaderInit(_shader);
        useTextureUnitInd(0, tex0->getId(), _shader, _tfo);        
        quad->draw(_tfo);
    }

    //---------------------------------------------------------------

    void SNTestQuadTexture::update(double time, double dt)
    {}
    
  }
