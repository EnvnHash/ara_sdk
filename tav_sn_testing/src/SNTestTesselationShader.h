//
// SNTestTesselationShader.h
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
    class SNTestTesselationShader : public SceneNode
    {
    public:
        SNTestTesselationShader(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTestTesselationShader();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void initTessShdr();
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};

    private:
        ShaderCollector*				shCol;
        Shaders*            tessShader;
        Shaders*            colShader;
        GLMCamera*          fullScrCam;
        bool                isInited;
        double              lastTime = 0.0;
        VAO*                part;
    };
}
