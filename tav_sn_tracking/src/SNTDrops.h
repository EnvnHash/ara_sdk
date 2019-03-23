//
// SNTDrops.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <GLUtils/GLSL/GLSLParticleSystem.h>
#include <NiTE/NISkeleton.h>
#include <KinectInput/KinectInput.h>
#include <Shaders/ShaderCollector.h>
#include <SceneNode.h>
#include <PAudio.h>

namespace tav
{
    class SNTDrops : public SceneNode
    {
    public:
        SNTDrops(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTDrops();
        
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};

    private:
        GLSLParticleSystem* ps;
        GLSLParticleSystem::EmitData data;
        GLSLParticleSystem::EmitData oldData;
        NISkeleton*         nis;
        KinectInput*        kin;
        PAudio*             pa;
        TextureManager* 	tex0;
        ShaderCollector*				shCol;

        bool                isInited=false;
        glm::vec3           emitPos;
        glm::vec3           emitDir;
        glm::vec3           rightH, leftH, rightHip, center, res;
        glm::vec3*          posSel;
        glm::vec4*			chanCols;

        int*                tex;
        
        float               distHandHip;
        float               distHandOffset;
        float               distHandScale;
        float               sizeMod;
        float               sizeRand;

        int                 frameNr = 0;
        int                 frameCount = 0;
        int                 lastBlock = 0;
        
        bool                repeat = false;
    };
}
