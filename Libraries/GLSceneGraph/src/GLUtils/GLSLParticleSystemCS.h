//
//  GLSLGLSLParticleSystemCSCS.h
//
//  Created by Sven Hahne, last modified on 22.08.17
//

#pragma once

#include <Shaders/ShaderUtils/ShaderBuffer.h>
#include <Utils/NoiseTexNV.h>
#include <glb_common/glb_common.h>

namespace ara {
struct ShaderParams {
    glm::mat4 ModelView;
    glm::mat4 ModelViewProjection;
    glm::mat4 ProjectionMatrix;
    glm::vec4 attractor;
    uint      numParticles;
    float     spriteSize;
    float     damping;
    float     initAmt;
    float     noiseFreq;
    float     noiseStrength;
    ShaderParams()
        : spriteSize(0.015f), attractor(0.0f, 0.0f, 0.0f, 0.0f), damping(0.95f), initAmt(0.f), noiseFreq(10.0f),
          noiseStrength(0.001f) {}
};

class GLSLParticleSystemCS {
public:
    GLSLParticleSystemCS(size_t size, const char* shaderPrefix);
    ~GLSLParticleSystemCS();

    void        loadShaders();
    void        reset(glm::vec3 size);
    void        update();
    std::string initShdr();
    std::string getShaderParUBlock();

    size_t                        getSize() { return m_size; }
    GLuint                        getNoiseTex() { return m_noiseTex; }
    ara::ShaderBuffer<glm::vec4>* getPosBuffer() { return m_pos; }
    ara::ShaderBuffer<glm::vec4>* getVelBuffer() { return m_vel; }
    ara::ShaderBuffer<uint32_t>*  getIndexBuffer() { return m_indices; }

private:
    GLuint       createShaderPipelineProgram(GLuint target, const char* src);
    ShaderParams mShaderParams;

    size_t                        m_size;
    ara::ShaderBuffer<glm::vec4>* m_pos;
    ara::ShaderBuffer<glm::vec4>* m_init_pos;
    ara::ShaderBuffer<glm::vec4>* m_vel;
    ara::ShaderBuffer<uint32_t>*  m_indices;

    GLuint m_programPipeline;
    GLuint m_updateProg;

    GLuint      m_noiseTex;
    int         m_noiseSize;
    NoiseTexNV* nt;
    const char* m_shaderPrefix;
    const int   work_group_size;
};
}  // namespace ara
