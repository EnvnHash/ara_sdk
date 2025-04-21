#pragma once

#include "SceneNodes/SNGizmoAxis.h"

namespace ara {

class SNGizmoTransAxis : public SNGizmoAxis {
public:
    SNGizmoTransAxis(sceneData* sd = nullptr);
    ~SNGizmoTransAxis() = default;

    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr);

private:
    std::unique_ptr<VAO> m_gizVao[2];
};

}  // namespace ara