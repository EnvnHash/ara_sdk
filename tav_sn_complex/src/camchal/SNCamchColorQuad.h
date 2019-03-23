//
// SNCamchColorQuad.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include <Shaders/Shaders.h>

namespace tav
{
    class SNCamchColorQuad : public SceneNode
    {
    public:
        SNCamchColorQuad(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNCamchColorQuad();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        glm::vec4*	chanCols;
        Quad*   	quad;
        ShaderCollector*				shCol;
        Shaders* 	stdColShdr;
        float		alpha=0.f;
    };
}
