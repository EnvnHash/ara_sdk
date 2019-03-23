//
// SNTFluid.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <GeoPrimitives/QuadArray.h>
#include <KinectInput/KinectInput.h>
#include <PAudio.h>


namespace tav
{
    class SNTFluid : public SceneNode
    {
    public:
        SNTFluid(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTFluid();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        bool            inited = false;
        
        GLSLFluid*      fluidSim;
        QuadArray*      quadAr;
        KinectInput*    kin;
        NISkeleton*     nis;
        Shaders*        shdr;
        PAudio*         pa;
        kinectMapping*  kinMapping;
        ShaderCollector*				shCol;

        glm::vec4*		chanCols;
        glm::vec2*           oldPos;
        glm::vec2            forceScale;
        glm::vec2 rightH, leftH, velL, velR;
        glm::vec2 oldRightH, oldLeftH;
        
        float           alphaScale;
        float           posScale;
        float           eRad = 3.f;
        float           eTemp = 10.f;
        float           eDen = 1.f;
        float           ampScale;
        
        GLint           drawTex = 0;
        unsigned int    flWidth;
        unsigned int    flHeight;
        int             lastBlock = -1;
        int             frameNr = -1;
        
        bool            texInited = false;
    };
}
