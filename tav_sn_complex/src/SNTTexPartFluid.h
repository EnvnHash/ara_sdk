//
// SNTTexPartFluid.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <opencv2/core.hpp>

#include <GLUtils/GLSL/FastBlurMem.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <GLUtils/GLSL/GLSLParticleSystem.h>
#include <GLUtils/GLSL/GLSLParticleSystem2.h>
#include <GLUtils/GLSL/GLSLParticleSystemFbo.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include <GLUtils/FBO.h>
#include <OpenCvUtils/CvFxChain.h>
#include <KinectInput/KinectReproTools.h>
#include <GLUtils/GWindowManager.h>
#include <Communication/OSC/OSCData.h>
#include <PAudio.h>

#include <SceneNode.h>


namespace tav
{
    class SNTTexPartFluid : public SceneNode
    {
    public:
        SNTTexPartFluid(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTTexPartFluid();
        
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);
        
    private:
        GWindowManager*                 winMan;
        OSCData*                        osc;
        KinectInput*                    kin;
        KinectReproTools*               kinRepro;
        
        GLSLFluid*                      fluidSim;
        GLSLParticleSystemFbo*          ps;
        GLSLParticleSystem::EmitData    data;
        GLSLOpticalFlow*                optFlow;

        ShaderCollector*   				shCol;
        Shaders*                        colShader;
        Shaders*                        texShader;
        Shaders*                        quadShader;
        FastBlurMem*                    fBlur;
        
        Quad*                           quad;
        
        TextureManager*                 litSphereTex;
        TextureManager*                 partTex;
        TextureManager*                 partTexNorm;
                
        bool                            inited;
        bool                            psInited;
        bool                            kParInited = false;
        bool                            hFlip;
        
        int                             nrPartLeaves;
        int                             maxNrPart;
        int                             nrForceJoints;
        int                             frameNr;
        int                             kinTexPtr=0;
        int                             nrPartTex;
        int                             lastFBlurFrame = -1;
        int                             emitPartPerUpdate;
        int                             kinDeviceNr;

        double                          mouseX = 0;
        double                          mouseY = 0;
        
        float                           flWidth;
        float                           flHeight;
        
        float                           scaleFact;
        float                           zPos;
        float                           transX;
        float                           transY;
                
        glm::vec2                       oldM;
        glm::vec2                       forceScale;
        
        glm::vec3                       kCamPos;
        glm::vec3                       camPos;
        
        std::vector<std::string>        args;
        std::string                     calibFileName;
        
        KinectReproTools::mode          actMode;
        
    };
}
