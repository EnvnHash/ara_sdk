//
// Created by user on 13.01.2021.
//

#pragma once

#include "Image.h"

namespace ara {

class Gizmo;

class GizmoAxisLabel : public Image {
public:
    GizmoAxisLabel();
    ~GizmoAxisLabel() override = default;

    void init() override;
    bool draw(uint32_t& objId) override;
    void pushVaoUpdtOffsets() override;

    void mouseIn(hidData& data) override;
    void mouseOut(hidData& data) override;
    void mouseDrag(hidData& data) override;
    void mouseDown(hidData& data) override;
    void mouseUp(hidData& data) override;

    bool updateCamFade() const;
    void setGizmoParent(Gizmo* gizmo);

    void setAxisFlag(TrackBallCam::snap axis) { m_gizAxis = axis; }
    void setHasLabel(bool val) { m_hasLabel = val; }
    [[nodiscard]] bool hasLabel() const { return m_hasLabel; }

    int m_presetObjId = 0;

private:
    Gizmo*             m_gizmo                  = nullptr;
    TrackBallCam::snap m_gizAxis                = TrackBallCam::snap::x_pos;
    bool               m_resetExcludeFromStyles = false;
    bool               m_hasLabel               = false;
    bool               m_leftPressed            = false;  // additional check for avoid errors with hid
                                                          // blocking during camera animation
    glm::vec2          m_mousePosRel = glm::vec2{0.f};
    glm::vec2          m_rotScale    = glm::vec2{1.f};
    AnimVal<glm::vec3> m_modelCamTransAnim;
};

}  // namespace ara
