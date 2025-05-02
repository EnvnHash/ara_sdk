#pragma once

#include "SceneNodes/SNGridFloor.h"
#include "SceneNodes/SceneNode.h"

namespace ara {

class SNGridFloorAxes : public SNGridFloor {
public:
    SNGridFloorAxes(sceneData* sd = nullptr);
    void update(double time, double dt, CameraSet* cs) override;
    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr) override;
    void setBasePlane(basePlane bp) {
        m_basePlane   = bp;
        m_rotationSet = false;
    }

    std::array<glm::vec4, 2> m_lineCol = {glm::vec4(1.f, 0.f, 0.f, 0.6f), glm::vec4(0.f, 0.f, 1.f, 0.6f)};

private:
    std::unique_ptr<VAO> m_lineVao = nullptr;
};

}  // namespace ara