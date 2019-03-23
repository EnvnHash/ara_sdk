//
// SNTestSkyBox.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GLUtils/SkyBox.h>

namespace tav
{
    class SNTestSkyBox : public SceneNode
    {
    public:
        SNTestSkyBox(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTestSkyBox();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
        
    private:
        SkyBox*             skyBox;
        bool                isInited;
        double              lastTime = 0.0;
    };
}
