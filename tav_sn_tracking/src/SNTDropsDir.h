//
// SNTDropsDir.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <GLUtils/GLSL/GLSLParticleSystem.h>
#include <NiTE/NISkeleton.h>
#include <SceneNode.h>
#include <KinectInput/KinectInput.h>
#include <PAudio.h>

namespace tav
{
    class SNTDropsDir : public SceneNode
    {
    public:
        SNTDropsDir(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTDropsDir();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};

    private:
        GLSLParticleSystem* ps;
        GLSLParticleSystem::EmitData data;
        GLSLParticleSystem::EmitData oldData;
        KinectInput*        kin;
        NISkeleton*         nis;
        PAudio*             pa;
        TextureManager* 	tex0;
        ShaderCollector*				shCol;

        bool                isInited=false;
        bool                repeat = false;
        
        glm::vec3           emitPos;
        glm::vec3           emitDir;
        glm::vec3           rightH, leftH, rightHip, center, res;
        glm::vec4*			chanCols;

        float               dirSpeed;
        float               angle;

        int                 frameNr = 0;
        int                 lastBlock = 0;
    };
}
