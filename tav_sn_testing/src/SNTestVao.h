//
// SNTestVao.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include "GLUtils/GLMCamera.h"
#include "GLUtils/VAO.h"

namespace tav
{
    class SNTestVao : public SceneNode
    {
    public:
        SNTestVao(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTestVao();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};

    private:
        Shaders*            colShader;
        GLMCamera*          fullScrCam;
        bool                isInited;
        double              lastTime = 0.0;
        VAO*                part;
    };
}
