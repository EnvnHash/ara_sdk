//
//  3DNoiseTexgen.h
//
//  Created by Sven Hahne on 18/5/15.
//

#pragma once

#include <GlbCommon/GlbCommon.h>
#include <Shaders/ShaderCollector.h>

namespace ara {

class FBO;
class sceneData;
class Quad;

class Noise3DTexGen {
public:
    Noise3DTexGen(const sceneData* scd, bool color, int nrOctaves, glm::ivec3 size, glm::vec3 scale);
    void drawNoise(std::unique_ptr<Quad>& quad, float zPos, int nrOctaves);
    void blendHorizontal(std::unique_ptr<FBO>& fbo, Shaders* xBlendShaderH, std::unique_ptr<Quad>& quad, float zPos, const glm::ivec3& size);
    void blendVertical(std::unique_ptr<FBO>& fbo, std::unique_ptr<FBO>& fboH, Shaders* xBlendShaderH, std::unique_ptr<Quad>& quad, float zPos, const glm::ivec3& size);
    void initShdr();

    [[nodiscard]] Shaders* initBlendShdrH() const;
    [[nodiscard]] Shaders* initBlendShdrV() const;
    [[nodiscard]] GLuint   getTex() const;

    std::unique_ptr<FBO> fbo;

private:
    Shaders*         m_noiseShdr = nullptr;
    Shaders*         m_stdTexShdr = nullptr;
    ShaderCollector& m_shCol;
    GLBase*          m_glbase = nullptr;

    float m_scaleX{};
    float m_scaleY{};
    float m_scaleZ{};

    int   m_width = 0;
    int   m_height = 0;
    int   m_depth = 0;
    int   m_nrLoops = 0;
    short m_nrParallelTex = 0;
};
}  // namespace ara
