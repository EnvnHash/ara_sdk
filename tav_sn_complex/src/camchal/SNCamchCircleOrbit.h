//
// SNCamchCircleOrbit.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//
#pragma once


#include <iostream>

#include <headers/gl_header.h>
#include <SceneNode.h>
#include <GeoPrimitives/Circle.h>
#include <GeoPrimitives/Quad.h>
#include <Communication/OSC/OSCData.h>
#include <GLUtils/GLSL/FastBlurMem.h>
#include <GLUtils/FBO.h>
#include <GLUtils/TextureManager.h>
#include <Shaders/Shaders.h>
#include <PAudio.h>

namespace tav
{
    class SNCamchCircleOrbit : public SceneNode
    {
    public:
        SNCamchCircleOrbit(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNCamchCircleOrbit();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void initShdr(TFO* _tfo);
        void initDepthShdr();
        void initBlurOverlayShdr();
    
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};

    private:
        Circle**    circs;
        FBO*		circFbo;
        FBO*		depthBlurFbo;
        OSCData*    osc;
        Quad* 		quad;
        PAudio*     pa;
        ShaderCollector*				shCol;
        Shaders*    stdTexAlpha;
        Shaders*    depthShdr;
        Shaders*    recShdr;
        Shaders*	blurOvr;
        TextureManager* litTex;
        TextureManager* cubeTex;
        FastBlurMem*		blur;
        glm::vec4*	chanCols;

        float       yAmp;
        float       circBaseSize;
        float       rotSpeed;
        float       depthScale;
        float       partOffs;
        float       scaleAmt;
        float       bright;
        float       timeIncr=0;
        float*		rotDir;
        float 		propo;
        float 		zDepth = 40.f;
        float 		zSpeed = 0.04f;
        float 		overBright = 0.28f;
        float 		backAlpha = 0.4f;
        float 		alpha = 0.f;


        int 		nrVar;
        int         nrPartCircs;
        int         nrInst;
        int         lastBlock = -1;

        bool        inited;
    };
}
