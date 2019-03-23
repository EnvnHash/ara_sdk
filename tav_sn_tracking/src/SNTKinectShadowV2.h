//
// SNTKinectShadowV2.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include <GLUtils/TextureManager.h>
#include <Shaders/Shaders.h>
#include <KinectInput/KinectInput.h>
#include <KinectInput/KinectReproTools.h>
#include <Shaders/ShaderCollector.h>
#include <GLUtils/GLSL/FastBlurMem.h>

namespace tav
{
    class SNTKinectShadowV2 : public SceneNode
    {
    public:
        SNTKinectShadowV2(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTKinectShadowV2();
    
        void initShdr();
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
    private:
        FastBlurMem*		blur;
        KinectInput*        kin;
        KinectReproTools*	kinRepro;
        Quad*               quad;
        Shaders*            shdr;
        Shaders*			renderShdr;
        OSCData*			osc;
        ShaderCollector*				shCol;

        int                 frameNr = -1;
        GLint*				transTexId;

    	float				blurAlpha=0.f;
    	float				blurBright=1.f;
    	float				alpha=0.f;
    	float				offsScale=1.f;

        glm::mat4 			transMat;
    };
}
