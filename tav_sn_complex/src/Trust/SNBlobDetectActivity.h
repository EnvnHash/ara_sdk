//
// SNCamchSimpTextur.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <GeoPrimitives/Quad.h>
#include <Shaders/Shaders.h>
#include <VideoTextureCvActRange.h>
#include <SceneNode.h>


namespace tav
{
    class SNBlobDetectActivity : public SceneNode
    {
    public:
    	SNBlobDetectActivity(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNBlobDetectActivity();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};

    private:
        VideoTextureCvActRange*		vidRange;
    	Quad*                   	quad;
    	Shaders*					stdTex;
        ShaderCollector*			shCol;

    	int							actUplTexId = 0;
    	int							lastTexId = -1;
    };
}
