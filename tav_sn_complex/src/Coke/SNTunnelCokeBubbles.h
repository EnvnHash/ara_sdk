//
// SNTunnelCokeBubbles.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <string>

#include <KinectInput/KinectInput.h>

#include <Communication/OSC/OSCData.h>
#include <GeoPrimitives/Quad.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include <GLUtils/GLSL/GLSLParticleSystemFbo.h>
#include <Shaders/Shaders.h>
#include <SceneNode.h>

namespace tav
{
    class SNTunnelCokeBubbles : public SceneNode
    {
    public:
        std::string samplerNames[4] = { "pos_tex", "vel_tex", "col_tex", "aux0_tex" };
        enum envMapType { ONLY_ENV=0, ENV_REFR=1 };

        SNTunnelCokeBubbles(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTunnelCokeBubbles();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void initQuadShader(unsigned int nrCams);
        
        void setEmitTex(GLuint _emitTexID, int width, int height);
        void setVelTex(GLuint _velTexID);
        void uploadNisImg(int device);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};

    private:
        GLSLFluid*                      fluidSim;
        GLSLParticleSystemFbo*          ps;
        GLSLParticleSystemFbo::EmitData 	data;

        PingPongFbo*					partPP;
        KinectInput*                    kin;

        OSCData*						osc;
        Quad*               			quad;
        Shaders*             			colShader;
        Shaders*             			texShader;
        Shaders*             			bubbleShader;
        Shaders*             			drawShaderQuad;
        Shaders*             			maskShader;
        ShaderCollector*				shCol;

        TextureManager*      			iceTex;
        TextureManager*      			cubeTex;
        TextureManager*      			bumpTex;
        TextureManager*      			backTex;
        TextureManager*      			litTex;
        TextureManager*      			bubbleTex;
        TextureManager*                 emitTex;

        std::string*         			dataPath;
        envMapType           			actMode;

        float							flWidth;
        float							flHeight;

        double                          lastTime = 0.0;
        double                          intrv;

        int*                            lastFrame;
        int*                            nisFrameNr;
        GLuint*                         nisTexId;

        int								maxNrPart;
    	int								emitTexID;
    	int								velTexID;
    	int								emitTexWidth;
    	int								emitTexHeight;
        int                             nrDevices;

        bool*                           nisTexInited;
        bool							firstInt = true;
        bool							inited = false;
    };
}
