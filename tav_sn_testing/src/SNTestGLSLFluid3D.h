//
//  SNGLSLFluidTest.h
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__SNTestGLSLFluid3D__
#define __Tav_App__SNTestGLSLFluid3D__

#pragma once

#include <iostream>
#include <stdio.h>
#include <math.h>

#include <SceneNode.h>
#include <GeoPrimitives/Circle.h>
#include <GLUtils/GLSL/GLSLFluid3D.h>
#include <Shaders/ShaderCollector.h>


namespace tav
{
    class SNTestGLSLFluid3D : public SceneNode
    {
    public:
        SNTestGLSLFluid3D(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTestGLSLFluid3D();
        
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
        void onCursor(double xpos, double ypos);
        
    private:
        bool            inited = false;
        
        Circle*         circle;
        GLSLFluid3D*    fluidSim;
        Shaders*        colShader;
        ShaderCollector*				shCol;
        glm::vec2       oldM;
        glm::vec2       forceScale;
        
        double          mouseX = 0;
        double          mouseY = 0;
        
        unsigned int    flWidth;
        unsigned int    flHeight;
        int             scrScale;
    };
}

#endif /* defined(__Tav_App__SNGLSLFluidTest__) */
