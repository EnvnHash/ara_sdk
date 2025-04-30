#pragma once

#include "SceneNodes/SceneNode.h"

namespace ara {

class SNGizmoAxis : public SceneNode {
public:
    SNGizmoAxis(sceneData* sd = nullptr);
    ~SNGizmoAxis() = default;

    virtual void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr) = 0;
    void         setGizmoColor(float r, float g, float b, float a) {
        gColor = glm::vec4(r, g, b, a);
    }

    std::unique_ptr<VAO>    gizVao;
    glm::vec4               gColor{};
    uint                    totNrIndices = 0;
    uint                    totNrPoints = 0;
    float                   emisBright;
};

}  // namespace ara