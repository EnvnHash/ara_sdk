//
// SNCamchSimpTextur.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>

#include <GeoPrimitives/QuadArray.h>
#include <GLUtils/GLSL/GodRays.h>
#include <Shaders/Shaders.h>
#include <SceneNode.h>

namespace tav
{
    class SNPseudo2DTo3D : public SceneNode
    {
    public:
    	SNPseudo2DTo3D(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNPseudo2DTo3D();

        void initShdr();
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        QuadArray*  		quad;
    	GodRays*			godRays;
    	FBO*				fbo;

        ShaderCollector*	shCol;

        Shaders* 			stdTexAlpha;
        Shaders* 			stdCol;
        Shaders* 			sphrShdr;
        TextureManager* 	tex0;

        glm::mat4 			scalePropoImg;
        double 				lastUpdt;

    	float				alpha=1.f;
    	float				xOffs=0.3f;
    	float				yOffs=0.f;
    	float				zOffs=1.f;

        float				exp=0.00235f;
        float				weight=5.53f;
        float				lightX=0.f;
        float				lightY=0.f;
    };
}
