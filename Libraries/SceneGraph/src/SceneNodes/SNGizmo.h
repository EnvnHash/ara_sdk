#pragma once

#include "SceneNodes/SNGizmoPlane.h"
#include "SceneNodes/SNGizmoRotAxis.h"
#include "SceneNodes/SNGizmoRotAxisLetter.h"
#include "SceneNodes/SNGizmoScaleAxis.h"
#include "SceneNodes/SNGizmoTransAxis.h"

namespace ara {

class SNGizmo : public SceneNode {
public:
    SNGizmo(transMode mode, sceneData* sd = nullptr);
    ~SNGizmo() = default;

    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* _tfo = nullptr);
    void selectAxis(size_t idx);
    void selectNextAxis();
    void selectPrevAxis();

    void                        setGizmoScreenSize(float size) { gizmoScreenSize = size; }
    std::vector<SNGizmoAxis*>*  getAxes() { return &gizmoAxes; }
    std::vector<SNGizmoPlane*>* getPlanes() { return &gizmoPlanes; }
    transMode&                  getTransMode() { return tMode; }

private:
    std::vector<SNGizmoAxis*>  gizmoAxes;
    std::vector<SNGizmoPlane*> gizmoPlanes;
    transMode                  tMode;

    float    gizmoScreenSize        = 0.f;
    float    planeAlpha             = 0.f;
    float    m_GizmoSizeRange[2]    = {0.1f, 0.2f};        // Size range for the gizmo
    uint64_t m_axisBitFlagOffset[4] = {2, 6, 10, 14};      // Trans, Scale, Rot, RotAxis
    uint64_t m_axisBitFlagMap[3]    = {2, 0, 1};           // Z, X, Y
    uint64_t m_planeBitFlagMap[6]   = {1, 2, 0, 2, 0, 1};  // Y, Z,  X, Z,  X, Y
    int      m_selectedAxis         = -1;
};

}  // namespace ara