//
//  SNTrustLogo.h
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__SNTrustLogo__
#define __Tav_App__SNTrustLogo__

#pragma once

#include <stdio.h>
#include <iostream>

#include <GeoPrimitives/Quad.h>
#include <GLUtils/Assimp/AssimpImport.h>
#include <GLUtils/GLSL/GodRays.h>
#include <GLUtils/TextureBuffer.h>
#include <GLUtils/SSAO.h>
#include <headers/gl_header.h>
#include <Shaders/ShaderCollector.h>
#include <SceneNode.h>
#include <VideoTextureCv.h>

namespace tav
{
    class SNTrustLogo : public SceneNode
    {
    public:
        SNTrustLogo(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTrustLogo();
        
        void initAlphaShdr();
        void initLitShader();
        void initColShader();
        void initCheapReflShader();
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
    private:
        AssimpImport*       aImport;
    	VideoTextureCv* 	vt;
    	GodRays*			godRays;
    	SceneNode*			logoNode;
    	SSAO*				ssao;

    	Quad*				quad;
    	Quad*				rawQuad;

    	FBO*				renderFbo;
    	FBO*				reflFbo;
    	FBO*				actScreenCpy;

        ShaderCollector*				shCol;

        Shaders*            shdr;
        Shaders*            litShdr;
        Shaders*            colShdr;
        Shaders*            stdColShdr;
        Shaders*            reflShdr;
        Shaders*            stdTexShdr;
        Shaders*            stdTexAlphaShdr;

        TextureManager*		litsphereTex;
        TextureManager*		cubeTex;
        TextureBuffer*		mQuad;

        glm::mat4           transMatr;
        glm::mat4			m_pvm;
        glm::vec4			lightCol;

        GLint				lastBoundFbo;

        int                 frameNr = -1;
        int					actUplTexId=0;

        float				reflAmt=0.15f;
        float				brightScale=0.8f;

        float				exp=0.00235f;
        float				dens=0.47f;
        float				decay=0.9999f;
        float				weight=5.53f;
        float				lightX=0.f;
        float				lightY=0.f;
        float				morph=0.f;
        float				rot=0.f;
        float				alpha=0.f;
        float				ssaoAlpha=0.f;
        float				grAlpha=1.f;
    };
}


#endif /* defined(__Tav_App__SNTrustLogo__) */
