//
// SNTestParticleCS.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>

#include <GLUtils/GLSL/GLSLParticleSystemCS.h>
#include <GLUtils/NoiseTexNV.h>
#include <GLUtils/UniformBlock.h>
#include <Shaders/ShaderCollector.h>
#include <SceneNode.h>

namespace tav
{

class SNTestParticleCS : public SceneNode
{
public:
	SNTestParticleCS(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
    ~SNTestParticleCS();

    void draw(void);
    std::string initUpdtPartShdr();
    void initDrawShdr();

    void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
    void cleanUp();
    void update(double time, double dt);
    void onKey(int key, int scancode, int action, int mods);

private:
	ShaderParams 			mShaderParams;
    ShaderCollector*		shCol;
	Shaders* 				mRenderProg;
    NoiseTexNV*				noiseTex;
    GLSLParticleSystemCS* 	mParticles;
    VAO*					testVAO;

    GLuint 					mUBO;
    GLint  					uboSize;
    GLuint 					uboIndex;

    bool 					mEnableAttractor;
    bool 					mAnimate;
    bool 					mReset;

    float					alpha;
    float					noiseFreq=10.f;
    float					noiseStren=0.001f;
    float					spriteSize;
    float					initAmt=0.f;

    float					propo;
    float 					mTime;
    float 					globalScale;
    unsigned int			nrCols;

    double              	lastTime = 0.0;
    double					lastPartUpdt = 0.0;

    glm::mat4 				view;
    glm::mat4 				proj;
    glm::vec4*				chanCols;

};

}
