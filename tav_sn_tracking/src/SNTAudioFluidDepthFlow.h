//
// SNTAudioFluidDepthFlow.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include <GLUtils/GLSL/FastBlurMem.h>
#include <GeoPrimitives/QuadArray.h>
#include <PAudio.h>
#include <KinectInput/KinectInput.h>

namespace tav
{
    class SNTAudioFluidDepthFlow : public SceneNode
    {
    public:
        SNTAudioFluidDepthFlow(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTAudioFluidDepthFlow();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        bool            inited = false;
        
        GLSLFluid*          fluidSim;
        QuadArray*          quadAr;
        KinectInput*        kin;
        kinectMapping*      kinMapping;
        Shaders*            shdr;
        PAudio*             pa;
        GLSLOpticalFlow*    optflow;
        FastBlurMem*        blur;
        ShaderCollector*				shCol;

        glm::vec2*          oldPos;
        glm::vec2           forceScale;
        glm::vec4*			chanCols;

        float               alphaScale;
        float               posScale;
        float               eRad = 3.f;
        float               eTemp = 10.f;
        float               eDen = 1.f;
        float               ampScale;
        
        GLint               drawTex = 0;
        unsigned int        flWidth;
        unsigned int        flHeight;
        int                 lastBlock = -1;
        int                 frameNr = -1;
        
        bool                texInited = false;
    };
}
