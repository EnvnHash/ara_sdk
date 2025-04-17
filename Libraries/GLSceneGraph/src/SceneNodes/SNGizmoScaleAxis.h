#pragma once

#include "SceneNodes/SNGizmoAxis.h"

namespace ara {

class SNGizmoScaleAxis : public SNGizmoAxis {
public:
    SNGizmoScaleAxis(sceneData* sd = nullptr);
    ~SNGizmoScaleAxis() = default;

    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr);
};

}  // namespace ara