//
// SNTunnelCoke.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2016 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>

#include <GLUtils/GWindowManager.h>
#include <GLUtils/SkyBoxBlend.h>
#include <SceneNode.h>

namespace tav
{
    class SNTunnelCoke : public SceneNode
    {
    public:
        SNTunnelCoke(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTunnelCoke();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);

    private:
        Quad*                           quad;
        Quad*                           rawQuad;
        camPar* 						this_cp;
        SkyBoxBlend*					skyBox;

        bool							inited = false;
        int                             frameNr = -1;
        int                             colFrameNr = -1;
    };
}
