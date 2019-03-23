//
// SNTunnelCokeIceCubes.h
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

#include <GLUtils/FBO.h>
#include <GeoPrimitives/Quad.h>
#include <Shaders/Shaders.h>
#include <Communication/OSC/OSCData.h>

#include <GLUtils/GLSL/GLSLParticleSystem2.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>

namespace tav
{
    class SNTunnelCokeIceCubes : public SceneNode
    {
    public:
        std::string samplerNames[4] = { "pos_tex", "vel_tex", "col_tex", "aux0_tex" };
        enum envMapType { ONLY_ENV=0, ENV_REFR=1 };

        SNTunnelCokeIceCubes(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTunnelCokeIceCubes();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void initQuadShader(unsigned int nrCams);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};

        void setEmitTex(GLuint _emitTexID);
        void setVelTex(GLuint _velTexID);

    private:
        OSCData* 						osc;
        GLSLParticleSystem2*          	ps;
        GLSLParticleSystem2::EmitData 	data;

        GLSLFluid*                      fluidSim;

        PingPongFbo*					partPP;
        FBO*							renderFbo;

        ShaderCollector*				shCol;

        Quad*               			quad;
        Shaders*             			colShader;
        Shaders*             			texShader;
        Shaders*             			bubbleShader;
        Shaders*             			drawShaderQuad;

        TextureManager*      			iceTex;
        TextureManager*      			bumpTex;

        TextureManager*      			cubeTex;
        TextureManager*      			backTex;
        TextureManager*      			allPicsTex;

        std::string*         			dataPath;
        std::string 					bigTexName;

        glm::vec2                       forceScale;
        glm::vec3						camPos;

        envMapType           			actMode;

        GLuint 							emitTexID;
        GLuint							velTexID;

        float							flWidth;
        float							flHeight;
        float							scaleFact;
        float 							destWidth;
        float							destHeight;

        double                          lastTime = 0.0;
        double                          intrv;

        int								maxNrPart;
        int								picWidth;
        int								picHeight;
        int 							totalNumPics;
        int                             nrPartH;
        int                             nrPartV;

        bool							firstInt = true;
        bool							inited = false;
    };
}
