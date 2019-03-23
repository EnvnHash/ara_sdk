//
// SNTestSphereTexture.h
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
#include <GeoPrimitives/Sphere.h>
#include <GeoPrimitives/CubeElem.h>
//#include "VideoTextureCapture.h"

#ifdef HAVE_OPENCV
#include <VideoTextureCv.h>
#endif

namespace tav
{
    class SNTestSphereTexture : public SceneNode
    {
    public:
        SNTestSphereTexture(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTestSphereTexture();
        
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void initShdr(camPar* cp);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        Quad*               	quad;
        Shaders*            	texShdr;
        Sphere*             	sphere;
        CubeElem*           	cubeElem;
        TextureManager* 		tex0;
        ShaderCollector*		shCol;

#ifdef HAVE_OPENCV
        VideoTextureCv*		    vt;
#endif

        std::string*        	dataPath;
        bool					isInited = false;
    	unsigned int			actUplTexId=0;

    };
}
