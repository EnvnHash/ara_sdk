//
//  TestOpticalFlow.h
//  tav_scene
//
//  Created by Sven Hahne on 13/7/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __tav_scene__TestOpticalFlow__
#define __tav_scene__TestOpticalFlow__

#pragma once

#include <stdio.h>
#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include <Shaders/ShaderCollector.h>
#include <GLUtils/TextureManager.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>

namespace tav
{
    class SNTestOpticalFlow : public SceneNode
    {
    public:
        SNTestOpticalFlow(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTestOpticalFlow();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        Quad*               quad;
        GLSLOpticalFlow*    flow;

        TextureManager*    	tex0;
        TextureManager*    	tex1;
        Shaders*            texShader;
        ShaderCollector*				shCol;
        TextureManager**    kinImgs;
        
        bool                bInit;
        int*                tex;
        int                 frameNr;
        int                 texPtr;
    };
}

#endif /* defined(__tav_scene__TestOpticalFlow__) */
