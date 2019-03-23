//
// SNTestRoom.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestRoom.h"

namespace tav
{
    SNTestRoom::SNTestRoom(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
        room = new Room(40.0f, 40.0f, 40.0f);
        
        testCube = new Cube();
        testCube->translate(0.f, 0.5f, -3.f);
    }
    

    SNTestRoom::~SNTestRoom()
    {
        delete testCube;
        delete room;
    }


    void SNTestRoom::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        room->draw(_tfo);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        testCube->draw(_tfo);
    }
    
    
    void SNTestRoom::update(double time, double dt)
    {
    }
    
    
 }
