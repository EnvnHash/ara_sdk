#pragma once

#include "Shaders/ShaderCollector.h"

namespace ara {

class GLBase;
class FBO;

class BoundingBoxer {
public:
    explicit BoundingBoxer(GLBase *glbase);

    void initShader();
    void begin();
    void end();

    void       sendModelMat(GLfloat *matPtr) { boxCalcShader->setUniformMatrix4fv("m_model", matPtr); }
    void       draw() {}
    glm::vec3 &getBoundMin() { return boundMin; }
    glm::vec3 &getBoundMax() { return boundMax; }
    glm::vec3 &getCenter() { return center; }
    Shaders   *getShader() { return boxCalcShader ? boxCalcShader : nullptr; }

private:
    std::unique_ptr<FBO> fbo;
    Shaders             *boxCalcShader = nullptr;
    ShaderCollector     &m_shCol;

    glm::vec3 boundMin{0.f};
    glm::vec3 boundMax{0.f};
    glm::vec3 center{0.f};

#ifdef ARA_USE_GLES31
    std::array<GLfloat, 24> result;
#else
    std::array<GLfloat, 6> result{};
#endif
};

}  // namespace ara