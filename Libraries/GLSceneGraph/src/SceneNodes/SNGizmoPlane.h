#pragma once

#include "SceneNodes/SceneNode.h"

using namespace glm;

namespace ara {
class SNGizmoPlane : public SceneNode {
public:
    SNGizmoPlane(sceneData* sd = nullptr);
    ~SNGizmoPlane();

    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr);
    void setGizmoUpperColor(float r, float g, float b, float a);
    void setGizmoLowerColor(float r, float g, float b, float a);

    void    setPlaneType(twPlane p) { m_planeType = p; }
    twPlane getPlaneType() { return m_planeType; }

private:
    std::array<std::array<std::unique_ptr<VAO>, 2>, 2> planeVao;
    std::array<glm::vec4, 2>                           gColor;
    float                                              emisBright;
    std::array<float, 2>                               ringInnerWidth{0.35f, 0.2f};
    std::array<float, 2>                               ringOuterWidth{0.45f, 0.6f};
    uint                                               nrBasePoints;
    twPlane                                            m_planeType = twPlane::xy;
};

}  // namespace ara