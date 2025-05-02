#pragma once

#include "SceneNodes/SceneNode.h"

namespace ara {

class SNGridFloor : public SceneNode {
public:
    SNGridFloor(sceneData* sd = nullptr);

    void update(double time, double dt, CameraSet* cs) override;
    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr) override;

    void setDepthMask(bool val) { m_depthMask = val; }

    virtual void setBasePlane(basePlane bp) {
        m_basePlane   = bp;
        m_rotationSet = false;
    }

protected:
    Quad*     m_quad              = nullptr;
    bool      m_depthMask         = true;
    bool      m_rotationSet       = false;
    float     m_ambientBrightness = 0.5f;
    glm::vec2 m_gridSize          = glm::vec2(40, 40);
    basePlane m_basePlane         = basePlane::xz;
};

}  // namespace ara