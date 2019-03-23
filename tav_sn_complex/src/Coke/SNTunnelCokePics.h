//
// SNTunnelCokePics.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <string>

#include <SceneNode.h>

#include <opencv2/core.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <GeoPrimitives/Quad.h>
#include <Shaders/Shaders.h>
#include <GLUtils/GLSL/GLSLParticleSystemFbo.h>

namespace tav
{
    class SNTunnelCokePics : public SceneNode
    {
    public:
        std::string samplerNames[4] = { "pos_tex", "vel_tex", "col_tex", "aux0_tex" };
        enum envMapType { ONLY_ENV=0, ENV_REFR=1 };
        enum drawType { POINTS = 0, QUADS = 1, DT_COUNT = 2 };

        SNTunnelCokePics(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTunnelCokePics();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void setupInstShader();
        void initQuadShader(unsigned int nrCams);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};

    private:
        ShaderCollector*				shCol;

        GLSLParticleSystemFbo*          ps;
        GLSLParticleSystemFbo::EmitData data;

        Quad*               			quad;
        Shaders*             			colShader;
        Shaders*             			texShader;
        Shaders*             			bubbleShader;
        Shaders*             			drawShaderQuad;

        TextureManager*      			cubeTex;
        TextureManager*      			bumpTex;
        TextureManager*      			backTex;
        TextureManager*      			litTex;
        TextureManager*      			bubbleTex;
        TextureManager*      			allPicsTex;

        std::string*         			dataPath;
        std::string 					bigTexName;
        envMapType           			actMode;

        float							flWidth;
        float							flHeight;

        double                          lastTime = 0.0;
        double                          intrv;

        int								maxNrPart;
        int								picWidth;
        int								picHeight;
        int 							totalNumPics;

        bool							firstInt = true;
        bool							inited = false;
    };
}
