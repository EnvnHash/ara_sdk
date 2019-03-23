//
// SNTNiteSilFluidPartSkel.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <math.h>

#include <GLUtils/GLSL/GLSLFluid.h>
#include <GLUtils/GLSL/GLSLParticleSystemFbo.h>
#include <GLUtils/GLSL/GLSLParticleSystem2.h>
#include <GLUtils/GLSL/GLSLOpticalFlow.h>
#include <SceneNode.h>
#include <KinectInput/KinectInput.h>

namespace tav
{
    class SNTNiteSilFluidPartSkel : public SceneNode
    {
    public:
        
         typedef struct {
            glm::vec2 pos;
        } thinPoint;
        
        SNTNiteSilFluidPartSkel(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTNiteSilFluidPartSkel();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void updateSil();
        void mouseTest(double time);
        void procUserMaps(uint8_t* _map, GLint tex, const nite::UserId* pLabels);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);
        
    private:
        GLSLFluid*                      fluidSim;
        GLSLOpticalFlow*                optFlow;

        GLSLParticleSystemFbo*          ps;
        GLSLParticleSystem::EmitData    data;

//        GLSLParticleSystem2*          ps;
//        GLSLParticleSystem2::EmitData    data;
        KinectInput*                    kin;

        Shaders*                        colShader;
        Shaders*                        texShader;
        Shaders*                        edgeDetect;
        ShaderCollector*				shCol;

        PingPongFbo*                    edgePP;
        TextureManager*                 userMapTuttiTex;
        TextureManager**                userMapTex;
        TextureManager**                kinColorTex;

        NISkeleton*                     nis;
        nite::JointType*                forceJoints;
        glm::vec2**                     oldJointPos;


        uint8_t**                       userMapConv;
        uint8_t*                        userMapConvTutti;

        Quad*                           rawQuad;

        bool                            inited;
        bool                            psInited;

        int                             nrParticle;
        int                             emitPartPerUpdate;
        int                             frameNr = -1;
        int                             colFrameNr = -1;
        int                             kinColorImgPtr = 0;
        int                             nrForceJoints;
        int                             thinDownSampleFact;

        unsigned int                    nrTestPart;
        const int                       maxNrUsers = 4;
        std::vector< std::vector<short> > userThinned;
        std::vector< thinPoint >        userThinnedPoints;
        std::vector< thinPoint >        userOldThinnedPoints;
        
        double                          mouseX = 0;
        double                          mouseY = 0;
        
        float                           flWidth;
        float                           flHeight;
        
        glm::vec2                       oldM;
        glm::vec2                       forceScale;
        glm::vec3                       massCenter;
        
    };
}
