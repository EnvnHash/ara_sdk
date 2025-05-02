#pragma once

#include "SceneNodes/SceneNode.h"

using namespace glm;

namespace ara {
class SNGizmoPlane : public SceneNode {
public:
    SNGizmoPlane(sceneData* sd = nullptr);

    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr) override;
    void setGizmoUpperColor(float r, float g, float b, float a);
    void setGizmoLowerColor(float r, float g, float b, float a);

    void    setPlaneType(twPlane p) { m_planeType = p; }
    twPlane getPlaneType() const { return m_planeType; }

private:
    std::array<std::array<std::unique_ptr<VAO>, 2>, 2>  m_planeVao{};
    std::array<glm::vec4, 2>                            m_gColor{};
    float                                               m_emisBright=0.3f;
    glm::vec2                                           m_ringInnerWidth{0.35f, 0.2f};
    glm::vec2                                           m_ringOuterWidth{0.45f, 0.6f};
    uint                                                m_nrBasePoints=20;
    twPlane                                             m_planeType = twPlane::xy;
};

}  // namespace ara
