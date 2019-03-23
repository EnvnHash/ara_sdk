//
// SNTestSkyBox.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  most basic vao test
//

#include "SNTestSkyBox.h"

namespace tav
{
    SNTestSkyBox::SNTestSkyBox(sceneData* _scd, std::map<std::string, float>* _sceneArgs) : SceneNode(_scd, _sceneArgs)
    {
        skyBox = new SkyBox((*scd->dataPath)+"textures/image_000_5.jpg", 1);
    }
    
    
    SNTestSkyBox::~SNTestSkyBox()
    { }
    
    
    void SNTestSkyBox::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        skyBox->draw(time, cp, _shader, _tfo);
    }
    
    
    void SNTestSkyBox::update(double time, double dt)
    {}
    
}
