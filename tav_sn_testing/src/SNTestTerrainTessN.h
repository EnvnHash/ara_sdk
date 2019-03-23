//
// SNTestTerrainTessN.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GLUtils/TessTerrain.h>

namespace tav
{
    class SNTestTerrainTessN : public SceneNode
    {
    public:
        SNTestTerrainTessN(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTestTerrainTessN();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        ShaderCollector*				shCol;
       TessTerrain*	terrain;
        float			heightScale=3.7f;
        float			animSpeed=0.f;
        float			skyTop=11.f;
        float			skyHeight=10.f;
        float 			ty=0.f;
        float			ry=0.f;
    };
}
