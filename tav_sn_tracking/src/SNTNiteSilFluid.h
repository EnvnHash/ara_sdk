//
// SNTNiteSilFluid.h
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
#include <GeoPrimitives/QuadArray.h>
#include <KinectInput/KinectInput.h>
#include <Communication/OSC/OSCData.h>

namespace tav
{
    class SNTNiteSilFluid : public SceneNode
    {
    public:
        SNTNiteSilFluid(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTNiteSilFluid();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
        void onCursor(double xpos, double ypos);

    private:
        OSCData*            osc;
        GLSLFluid*          fluidSim;
        GLSLOpticalFlow*    optFlow;
        QuadArray*          quadAr;
        Quad*               rawQuad;
        Quad*               rotQuad;
        KinectInput*        kin;
        NISkeleton*         nis;
        Shaders*            shdr;
        Shaders*            edgeDetect;
        PingPongFbo*        edgePP;
        TextureManager*     userMapTex;
        TextureManager**    kinColorTex;
        ShaderCollector*				shCol;

        uint8_t*            userMapConv;
        
        glm::vec2*          oldPos;
        glm::vec2           forceScale;
        glm::vec2           rightH, leftH, velL, velR;
        glm::vec2           oldRightH, oldLeftH;
        
        glm::vec2           oldM;
        glm::vec3           multCol;
        
        double              mouseX = 0;
        double              mouseY = 0;
        
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
        int                 colFrameNr = -1;
        int                 scrScale;
        int                 kinColorImgPtr = 0;

        bool                inited = false;
        bool                useHisto = false;
    };
}
