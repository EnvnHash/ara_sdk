//
//  3DNoiseTexgen.h
//
//  Created by Sven Hahne on 18/5/15.
//

#pragma once

#include <GLUtils/sceneData.h>
#include <GeoPrimitives/Quad.h>
#include <Shaders/ShaderCollector.h>
#include <Utils/FBO.h>
#include <glb_common/glb_common.h>

namespace ara {
class Noise3DTexGen {
public:
    Noise3DTexGen(sceneData* scd, bool color, int nrOctaves, glm::ivec3 size, glm::vec3 scale);

    void          initShdr();
    ara::Shaders* initBlendShdrH();
    ara::Shaders* initBlendShdrV();
    GLuint        getTex();

    std::unique_ptr<FBO> fbo;

private:
    Shaders*         m_noiseShdr = nullptr;
    Shaders*         m_stdTexShdr = nullptr;
    ShaderCollector* m_shCol = nullptr;
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
