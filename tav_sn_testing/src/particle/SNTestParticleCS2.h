//
// SNTestParticleCS2.h
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

class SNTestParticleCS2 : public SceneNode
{
public:
	SNTestParticleCS2(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
    ~SNTestParticleCS2();

    void draw(void);
  //  std::string initUpdtPartShdr();
    void initDrawShdr();

    void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
    void cleanUp();
    void update(double time, double dt);
    void onKey(int key, int scancode, int action, int mods);

private:
    GLSLParticleSystemCS* 	mParticles;
    NoiseTexNV*				noiseTex;
    VAO*					testVAO;
	ShaderParams 			mShaderParams;
    ShaderCollector*		shCol;
	Shaders* 				mRenderProg;
	TextureManager*			testEmitTex;

    int						mNumParticles;
    GLuint 					mUBO;
    GLuint 					mVBO;

    GLint  					uboSize;
    GLuint 					uboIndex;

    bool 					mEnableAttractor;
    bool 					mAnimate;
    bool 					mReset;

    float					alpha=0.f;
    float					spriteSize=0.006f;
    float					initAmt=0.f;


    float					propo;
    float 					mTime;
    float 					globalScale;
    int						oldReset=0;

    double              	lastTime = 0.0;
    double					lastPartUpdt = 0.0;
    double					lastEmit = 0.0;

    glm::mat4 				view;
    glm::mat4 				proj;
    glm::vec4*				chanCols;
    glm::vec4* 				initPars;

};

}
