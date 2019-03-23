//
// SNCamchBorders.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <Shaders/Shaders.h>
#include <GeoPrimitives/Quad.h>

namespace tav
{
    class SNCamchBorders : public SceneNode
    {
    public:
        SNCamchBorders(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNCamchBorders();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        ShaderCollector*		shCol;
        Quad*   				quad;
        VAO*					lines;
        Shaders* 				stdCol;
        std::vector<glm::vec3>	borderPos;
    };
}
