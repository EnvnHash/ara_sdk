//
//  SNKinectShadow.h
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__SNKinectShadow__
#define __Tav_App__SNKinectShadow__

#pragma once

#include <stdio.h>
#include <iostream>

#include <KinectInput/KinectInput.h>
#include <Shaders/ShaderCollector.h>

#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include <GeoPrimitives/QuadArray.h>
#include <GLUtils/GLSL/FastBlur.h>

namespace tav
{
    class SNKinectShadow : public SceneNode
    {
    public:
        SNKinectShadow(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNKinectShadow();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
    private:
        Quad*               quad;
        QuadArray*          quadAr;
        ShaderCollector*				shCol;

        KinectInput*        kin;
        FastBlur*           blur;
        
        int                 frameNr = -1;
        int                 width;
        int                 height;
        bool                updateProc = false;
        bool                inited = false;
    };
}


#endif /* defined(__Tav_App__SNKinectShadow__) */
