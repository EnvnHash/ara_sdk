#pragma once

#include "SceneNodes/SceneNode.h"

namespace ara {

class SNGizmoAxis : public SceneNode {
public:
    explicit SNGizmoAxis(sceneData* sd = nullptr);
    ~SNGizmoAxis() override = default;

    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr) override = 0;
    void setGizmoColor(float r, float g, float b, float a) { m_gColor = {r, g, b, a}; }

    std::unique_ptr<VAO>    m_gizVao;
    glm::vec4               m_gColor{};
    int32_t                 m_totNrIndices = 0;
    uint                    m_totNrPoints = 0;
    float                   m_emisBright = 0.3f;
};

}  // namespace ara