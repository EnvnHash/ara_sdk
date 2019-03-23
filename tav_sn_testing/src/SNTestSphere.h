//
// SNTestSphere.h
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
#include <GeoPrimitives/Sphere.h>
#include <GeoPrimitives/CubeElem.h>

namespace tav
{
    class SNTestSphere : public SceneNode
    {
    public:
        SNTestSphere(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTestSphere();
        
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        Quad*               quad;
        Shaders*            envShdr;
        Sphere*             sphere;
        CubeElem*           cubeElem;
        TextureManager* 	tex0;
        ShaderCollector*				shCol;

        std::string*        dataPath;
    };
}
