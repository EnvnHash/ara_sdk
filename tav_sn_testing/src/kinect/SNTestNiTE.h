//
// SNTestNiTE.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include <Shaders/Shaders.h>
#include <GLUtils/TextureManager.h>
#include "GLUtils/VAO.h"
#include <KinectInput/KinectInput.h>
#include <NiTE/NISkeleton.h>

#include <GLUtils/GWindowManager.h>


namespace tav
{
    class SNTestNiTE : public SceneNode
    {
    public:
        SNTestNiTE(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTestNiTE();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);

        void    uploadImg();
        GLuint  getTexId();
        
    private:

        bool            inited = false;
        bool            texInited = false;
        bool            generateMips = false;

        NISkeleton*     nis;
        KinectInput*    kin;
        ShaderCollector* shCol;

        Quad*           quad;
        Shaders*        shdr;
        
        GLuint          texId;
        GLuint          depthTexNr;
        GLuint          colorTexNr;
        GLuint          showTexNr = 0;
        
        int             frameNr = -1;
        int             depthFrameNr = -1;
        int             show;
        int             skelOffPtr;
        int             colorCount;
        int             resImgWidth;
        int             resImgHeight;
        int             mimapLevels = 0;

        glm::mat4       identMtr;
    };
}
