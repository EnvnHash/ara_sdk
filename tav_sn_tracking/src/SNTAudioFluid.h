//
// SNTAudioFluid.h
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
#include <PAudio.h>
#include <KinectInput/KinectInput.h>
#include <NiTE/NISkeleton.h>


namespace tav
{
    class SNTAudioFluid : public SceneNode
    {
    public:
        SNTAudioFluid(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTAudioFluid();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        bool            inited = false;
        
        GLSLFluid*      fluidSim;
        QuadArray*      quadAr;
        KinectInput*    kin;
        kinectMapping*  kinMapping;
        NISkeleton*     nis;
        Shaders*        shdr;
        PAudio*         pa;
        ShaderCollector*				shCol;

        glm::vec2*      oldPos;
        glm::vec2       forceScale;
        glm::vec2 		rightH, leftH, velL, velR;
        glm::vec2 		oldRightH, oldLeftH;
        glm::vec4*		chanCols;


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
