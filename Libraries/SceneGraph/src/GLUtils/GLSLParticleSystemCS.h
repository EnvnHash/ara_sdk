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
    glm::mat4 ModelView{};
    glm::mat4 ModelViewProjection{};
    glm::mat4 ProjectionMatrix{};
    glm::vec4 attractor{};
    uint      numParticles{};
    float     spriteSize = 0.015f;
    float     damping = 0.95f;
    float     initAmt = 0.f;
    float     noiseFreq = 10.f;
    float     noiseStrength = 0.001f;
};

class GLSLParticleSystemCS {
public:
    GLSLParticleSystemCS(const size_t size, const char* shaderPrefix);
    ~GLSLParticleSystemCS();

    void        loadShaders();
    void        reset(glm::vec3 size);
    void        update() const;
    static std::string initShdr();
    static std::string getShaderParUBlock();

    [[nodiscard]] size_t                   getSize() const { return m_size; }
    [[nodiscard]] GLuint                   getNoiseTex() const { return m_noiseTex; }
    [[nodiscard]] ShaderBuffer<glm::vec4>& getPosBuffer() { return m_pos; }
    [[nodiscard]] ShaderBuffer<glm::vec4>& getVelBuffer() { return m_vel; }
    [[nodiscard]] ShaderBuffer<uint32_t>&  getIndexBuffer() { return m_indices; }

private:
    [[nodiscard]] GLuint createShaderPipelineProgram(const GLuint target, const std::string& src) const;
    ShaderParams mShaderParams{};

    size_t                  m_size{};
    ShaderBuffer<glm::vec4> m_pos{};
    ShaderBuffer<glm::vec4> m_init_pos{};
    ShaderBuffer<glm::vec4> m_vel{};
    ShaderBuffer<uint32_t>  m_indices{};

    GLuint                      m_programPipeline = 0;
    GLuint                      m_updateProg = 0;
    GLuint                      m_noiseTex = 0;
    int                         m_noiseSize = 16;
    std::string                 m_shaderPrefix;
    const int                   work_group_size = 128;
    std::unique_ptr<NoiseTexNV> m_nt;
};
}  // namespace ara
