//
// SNAutumnLeaves.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <GLUtils/GLSL/GLSLParticleSystem.h>
#include <KinectInput/KinectInput.h>
#include <SceneNode.h>

namespace tav
{
    class SNAutumnLeaves : public SceneNode
    {
    public:
        SNAutumnLeaves(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNAutumnLeaves();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);
        
    private:
        KinectInput*                    kin;
        
        GLSLFluid*                      fluidSim;
        GLSLParticleSystem*             ps;
        GLSLParticleSystem::EmitData    data;
        
        ShaderCollector*    			shCol;

        Shaders*                        colShader;
        Shaders*                        texShader;
        Quad*                           quad;
        TextureManager*                 backTex;
        TextureManager**                texs;

        NISkeleton*                     nis;
        nite::JointType*                forceJoints;
        glm::vec2*                      oldJointPos;

        bool                            inited;
        bool                            psInited;
        int                             nrPartLeaves;
        int                 			nrForceJoints;

        unsigned int                    nrTestPart;
        
        double                          mouseX = 0;
        double                          mouseY = 0;
        
        float                           flWidth;
        float                           flHeight;

        glm::vec2                       oldM;
        glm::vec2                       forceScale;

    };
}
