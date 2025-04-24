//
//	SNParticles.h
//
//  Created by Sven Hahne, last modified on 22.08.17.
//

#pragma once

#include <SceneNodes/SceneNode.h>
#include <Shaders/ShaderCollector.h>
#include <Shaders/ShaderUtils/ShaderBuffer.h>
#include <Utils/NoiseTexNV.h>
#include <glb_common/glb_common.h>

#include "GLUtils/GLSLParticleSystemCS.h"

namespace ara {

class SNParticles : public SceneNode {
public:
    SNParticles(sceneData* sd = nullptr);
    ~SNParticles();

    void draw(void);
    void initShdr();
    std::string getVertShader();
    std::string getFragShader();
    void draw(double time, double dt, CameraSet* cs, Shaders* _shader, renderPass _pass, TFO* _tfo = nullptr);
    void cleanUp();
    void update(double time, double dt);
    void onKey(int key, int scancode, int action, int mods);

private:
    ShaderParams                          mShaderParams;
    ShaderCollector*                      shCol       = nullptr;
    Shaders*                              mRenderProg = nullptr;
    std::unique_ptr<GLSLParticleSystemCS> mParticles;
    std::unique_ptr<VAO>                  testVAO;

    const int mNumParticles;
    GLuint    mUBO;
    GLuint    mVBO;

    GLint  uboSize;
    GLuint uboIndex;

    GLfloat partColors[6][4];

    bool mEnableAttractor;
    bool mAnimate;
    bool mReset;

    float alpha;
    float noiseFreq;
    float noiseStren;
    float spriteSize;
    float initAmt;

    float propo;
    float mTime;
    float globalScale;

    int oldReset;
    int nrPartColors;

    double lastTime;
    double lastPartUpdt;

    glm::mat4  model;
    glm::mat4  view;
    glm::mat4  proj;
    glm::vec4* chanCols;
};

}  // namespace ara
