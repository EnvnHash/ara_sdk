//
// SNTestRoom.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GeoPrimitives/Cube.h>
#include <GeoPrimitives/Room.h>

namespace tav
{
    class SNTestRoom : public SceneNode
    {
    public:
        SNTestRoom(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTestRoom();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        Cube*   testCube;
        Room*   room;
        GLuint* textures;
    };
}
