#pragma once

#include "SceneNodes/SNGridFloor.h"

namespace ara {

class SNWorldAxes : public SNGridFloor {
public:
    SNWorldAxes(sceneData* sd = nullptr);
    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr);

    std::array<glm::vec4, 3> m_lineCol = {glm::vec4(1.f, 0.f, 0.f, 1.f), glm::vec4(0.f, 1.f, 0.f, 1.f),
                                          glm::vec4(0.f, 0.f, 1.f, 1.f)};

private:
    std::unique_ptr<VAO> m_lineVao = nullptr;
};

}  // namespace ara