//
// SNTPartLines.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <cmath>
#include <GLUtils/GLSL/GLSLParticleSystem.h>
#include <NiTE/NISkeleton.h>
#include <KinectInput/KinectInput.h>
#include <SceneNode.h>
#include <PAudio.h>

namespace tav
{
    class SNTPartLines : public SceneNode
    {
    public:
        SNTPartLines(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTPartLines();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
        
    private:
        GLSLParticleSystem*     ps;
        GLSLParticleSystem::EmitData data;
        TextureManager* 	tex0;
        ShaderCollector*				shCol;

        KinectInput*            kin;
        NISkeleton*             nis;
        PAudio*                 pa;
        bool                    isInited=false;
        glm::vec4*				chanCols;
        glm::vec3               emitPos;
        glm::vec3               emitDir;
        glm::vec3               head, foot;
        glm::vec3 rightH, leftH, center, res;
        glm::vec3               audioPos;
        glm::vec3               audioDir;
        float                   dirSpeed;
        
        int                     frameNr = 0;
        int                     lastBlock = 0;
        int                     partPerSamp;
        
        float                   offX;
        float                   offY;
        float                   dirOff;
        float                   pllPtrOffs;
        float                   pllRange;
        float*                  pllPtr;
        float                   angle;
        float                   length;
        float                   lengthHands;
        
        float                   headOffs;
        float                   headHeightScale;
        
        float                   medDt;
        float                   lastTime;
    };
}
