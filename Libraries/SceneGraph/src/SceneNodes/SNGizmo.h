#pragma once

#include "SceneNodes/SceneNode.h"

namespace ara {

class SNGizmoAxis;
class SNGizmoPlane;

class SNGizmo : public SceneNode {
public:
    explicit SNGizmo(transMode mode, sceneData* sd = nullptr);
    ~SNGizmo() override = default;

    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* _tfo = nullptr);
    void selectAxis(size_t idx);
    void selectNextAxis();
    void selectPrevAxis();

    void                        setGizmoScreenSize(const float size) { m_gizmoScreenSize = size; }
    std::vector<SNGizmoAxis*>*  getAxes() { return &m_gizmoAxes; }
    std::vector<SNGizmoPlane*>* getPlanes() { return &m_gizmoPlanes; }
    transMode&                  getTransMode() { return m_tMode; }

private:
    std::vector<SNGizmoAxis*>   m_gizmoAxes;
    std::vector<SNGizmoPlane*>  m_gizmoPlanes;
    transMode                   m_tMode;
    float                       m_gizmoScreenSize   = 0.f;
    float                       m_planeAlpha        = 0.f;
    glm::vec2                   m_GizmoSizeRange    = {0.1f, 0.2f};        // Size range for the gizmo
    std::array<uint64_t, 4>     m_axisBitFlagOffset = {2, 6, 10, 14};      // Trans, Scale, Rot, RotAxis
    std::array<uint64_t, 3>     m_axisBitFlagMap    = {2, 0, 1};           // Z, X, Y
    std::array<uint64_t, 6>     m_planeBitFlagMap   = {1, 2, 0, 2, 0, 1};  // Y, Z,  X, Z,  X, Y
    int                         m_selectedAxis      = -1;
};

}  // namespace ara