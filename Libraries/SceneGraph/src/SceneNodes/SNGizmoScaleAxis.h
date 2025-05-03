#pragma once

#include "SceneNodes/SNGizmoAxis.h"

namespace ara {

class SNGizmoScaleAxis : public SNGizmoAxis {
public:
    explicit SNGizmoScaleAxis(sceneData* sd = nullptr);
    ~SNGizmoScaleAxis() override = default;

    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr);
};

}  // namespace ara