//
// SNKinectGeoRotate.h
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
#include <Shaders/ShaderCollector.h>
#include <GLUtils/GWindowManager.h>
#include <KinectInput/KinectReproTools.h>

namespace tav
{
    class SNKinectGeoRotate : public SceneNode
    {
    public:
        SNKinectGeoRotate(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNKinectGeoRotate();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
    private:
        KinectInput*        kin;
        KinectReproTools*	kinRepro;
        Quad*               quad;
        Shaders*            shdr;
        ShaderCollector*	shCol;

    	float				kinRotX= 0.f;
    	float				kinRotY= 0.f;
    	float				kinRotZ= 0.f;

    	float				kinTransX= 0.f;
    	float				kinTransY= 0.f;
    	float				kinTransZ= 0.f;

    	float				screenScaleX = 1.f;
    	float				screenScaleY = 1.f;

    	float				screenTransX = 1.f;
    	float				screenTransY = 1.f;

    	float				pointSize= 1.f;
    	float				pointWeight= 1.f;

    	float				nearThres= 5000.f;
    	float				farThres= 1.f;

        int                 frameNr = -1;
        GLint*				transTexId=0;

        std::string			calibFileName;
        glm::mat4 			transMat;
        glm::mat4 			transMat2D;
    };
}
