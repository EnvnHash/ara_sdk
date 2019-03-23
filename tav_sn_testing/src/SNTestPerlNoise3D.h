//
// SNTestPerlNoise3D.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include <headers/gl_header.h>
#include <Shaders/ShaderCollector.h>
#include <GLUtils/FBO.h>
#include <Shaders/Shaders.h>
#include <GLUtils/Noise3DTexGen.h>


namespace tav
{
    class SNTestPerlNoise3D : public SceneNode
    {
    public:
        SNTestPerlNoise3D(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTestPerlNoise3D();
        
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        
        void initShdr();
        void initBlendShdr();
        void initTestShader();
        
        void drawManual(TFO* _tfo);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        Quad*   			quad;
        Noise3DTexGen*		noiseTex;
        FBO*                fbo;
        FBO*                xBlendFboH;
        FBO*                xBlendFboV;
        
        Shaders*            noiseShdr;
        Shaders*            xBlendShaderH;
        Shaders*            xBlendShaderV;
        Shaders*            stdTexShdr;
        Shaders*            colShdr;
        Shaders*            test;
        Shaders*            testShdr;

        ShaderCollector*    shCol;
        
        GLfloat*            zPos;
        float               scaleX;
        float               scaleY;
        float               scaleZ;
        
        int                 width;
        int                 height;
        int                 depth;
        int                 nrLoops;
        int					nrOctaves;
        short               nrParallelTex;
        bool 				inited = false;
    };
}
