//
// SNTThoughtBubble.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <KinectInput/KinectInput.h>
#include <NiTE/NISkeleton.h>
#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>

namespace tav
{
    class SNTThoughtBubble : public SceneNode
    {
    public:
        SNTThoughtBubble(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTThoughtBubble();
        
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
        
    private:
        KinectInput*            kin;
        NISkeleton*             nis;
        TextureManager*         bubbleTex;
        Quad*                   quad;
        Shaders*                texShader;
        ShaderCollector*				shCol;

        bool                    isInited;
        
        glm::mat4               modMatr;
        glm::mat4               kinDebugMatr;
        glm::vec3               head;
        glm::vec3               headOffset;
        
        int                     frameNr = 0;
        int                     colFrameNr = 0;
        
        float                   texScale;
                
        float                   medDt;
        float                   lastTime;
    };
}
