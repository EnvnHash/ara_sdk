//
// SNTNiteSilhoutte.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GLUtils/FBO.h>
#include <GeoPrimitives/Quad.h>
#include <KinectInput/KinectInput.h>

namespace tav
{
    class SNTNiteSilhoutte : public SceneNode
    {
    public:
        SNTNiteSilhoutte(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTNiteSilhoutte();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        KinectInput*        kin;
        NISkeleton*         nis;
        Quad*               quad;
        Shaders*            shdr;
        Shaders*            edgeDetect;
        FBO*                edgeFBO;
        TextureManager*     userMapTex;
        ShaderCollector*				shCol;

        uint8_t*            userMapConv;
        int                 frameNr = -1;
        bool                inited = false;
    };
}
