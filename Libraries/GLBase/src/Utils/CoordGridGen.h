#pragma once

#include "GLBase.h"

namespace ara {

class CoordGridGen {
public:
    CoordGridGen(glm::ivec2 texSize);
    ~CoordGridGen() = default;

    Shaders *initCoordGenShader();

private:
    ShaderCollector m_shCol;
};

}  // namespace ara