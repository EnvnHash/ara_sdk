#pragma once

#include "SceneNodes/SNGizmoAxis.h"

namespace ara {

class SNGizmoRotAxisLetter : public SNGizmoAxis {
public:
    SNGizmoRotAxisLetter(sceneData* sd = nullptr);

    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr) override;

    std::vector<GLfloat> m_positions{0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, -1.f, 0.f, 0.f, 0.f, 0.f};
    std::vector<GLfloat> m_colors;
};

}  // namespace ara