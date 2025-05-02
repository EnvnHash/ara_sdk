//
//	SNParticles.h
//
//  Created by Sven Hahne, last modified on 22.08.17.
//

#pragma once

#include <GLUtils/GLSLParticleSystemCS.h>
#include <SceneNodes/SceneNode.h>

namespace ara {

class SNParticles : public SceneNode {
public:
    explicit SNParticles(sceneData* sd = nullptr);
    ~SNParticles() override;

    void draw();
    void initShdr();
    static std::string getVertShader();

    static std::string getFragShader();
    void draw(double time, double dt, CameraSet* cs, Shaders* _shader, renderPass _pass, TFO* _tfo = nullptr) override;
    void cleanUp();
    void update(double time, double dt);
    void onKey(int key, int scancode, int action, int mods) override;

private:
    ShaderParams                          m_shaderParams{};
    ShaderCollector*                      m_shCol       = nullptr;
    Shaders*                              m_renderProg = nullptr;
    std::unique_ptr<GLSLParticleSystemCS> m_particles;
    std::unique_ptr<VAO>                  m_testVAO;

    const int m_numParticles = 1 << 17;
    GLuint    m_UBO = 0;
    GLuint    m_VBO = 0;

    GLint  m_uboSize = 0;
    GLuint m_uboIndex  = 0;

    std::array<glm::vec4, 6> m_partColors{};

    bool mEnableAttractor = true;
    bool mReset = false;

    float m_alpha = 1.f;
    float m_noiseFreq = 10.f;
    float m_noiseStren = 0.002f;
    float m_spriteSize = 0.025f;
    float m_initAmt = 0.f;

    float m_propo = 0.f;
    float m_time = 0.f;
    float m_globalScale = 16.f;

    int32_t m_oldReset = 0;
    int32_t m_nrPartColors = 6;

    double m_lastTime = 0.0;
    double m_lastPartUpdt = 0.0;

    glm::mat4  m_model{};
    glm::mat4  m_view{};
    glm::mat4  m_proj{};
    glm::vec4* m_chanCols= nullptr;
};

}  // namespace ara
