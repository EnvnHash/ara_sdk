#include "SceneNodes/SNGizmo.h"

#include <nameof.hpp>

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SNGizmo::SNGizmo(transMode _mode, sceneData* sd)
    : SceneNode(sd), tMode(_mode), gizmoScreenSize(0.15f), planeAlpha(0.8f) {
    m_staticNDCSize = 75.f;  // in pixels
                             // m_staticNDCSize = 0.15f;
    m_depthTest     = true;
    m_cullFace      = true;
    m_nodeType      = GLSG_GIZMO;
    m_emptyDrawFunc = true;

    setName(getTypeName<SNGizmo>() + "_" + string(nameof::nameof_enum(tMode)));
    string  gizmoPlaneNames[3] = {getTypeName<SNGizmo>() + "_Y_Z", getTypeName<SNGizmo>() + "_X_Z",
                                  getTypeName<SNGizmo>() + "_X_Y"};
    twPlane gizmoPlaneTypes[3] = {twPlane::yz, twPlane::xz, twPlane::xy};

    // init Gizmo Planes
    if (tMode != transMode::rotate && tMode != transMode::rotate_axis && tMode != transMode::passive) {
        for (uint i = 0; i < 3; i++) {
            gizmoPlanes.emplace_back(dynamic_cast<SNGizmoPlane *>(addChild(make_unique<SNGizmoPlane>())));
            gizmoPlanes.back()->setVisibility(true);
            gizmoPlanes.back()->setName(gizmoPlaneNames[i]);
            gizmoPlanes.back()->setPlaneType(gizmoPlaneTypes[i]);
            gizmoPlanes.back()->setAllowDragSelect(true);
        }
    }

    switch (tMode) {
        case transMode::translate:
            for (uint i = 0; i < 3; i++) gizmoAxes.push_back(dynamic_cast<SNGizmoAxis *>(addChild(make_unique<SNGizmoTransAxis>())));
            m_nameFlag |= GLSG_TRANS_GIZMO;
            break;

        case transMode::passive:
            for (uint i = 0; i < 3; i++) {
                gizmoAxes.push_back(dynamic_cast<SNGizmoAxis *>(addChild(make_unique<SNGizmoTransAxis>())));
                gizmoAxes[i]->m_selectable = false;
            }
            m_nameFlag |= GLSG_PASSIVE_GIZMO;
            m_staticNDCSize = 0.f;
            break;

        case transMode::rotate:
            for (uint i = 0; i < 3; i++) {
                gizmoAxes.push_back(dynamic_cast<SNGizmoAxis*>(addChild(make_unique<SNGizmoRotAxis>())));
            }
            m_nameFlag |= GLSG_ROT_GIZMO;
            break;

        case transMode::rotate_axis:
            for (uint i = 0; i < 3; i++) {
                gizmoAxes.push_back(dynamic_cast<SNGizmoAxis *>(addChild(make_unique<SNGizmoRotAxisLetter>())));
            }
            m_nameFlag |= GLSG_ROT_AXIS_GIZMO;
            break;

        case transMode::scale:
            for (uint i = 0; i < 3; i++) {
                gizmoAxes.push_back(dynamic_cast<SNGizmoAxis *>(addChild(make_unique<SNGizmoScaleAxis>())));
            }
            m_nameFlag |= GLSG_SCALE_GIZMO;
            break;
        case transMode::none: break;
        default: break;
    }

    string gizmoNames[3] = {"Gizmo_Z", "Gizmo_X", "Gizmo_Y"};

    for (uint i = 0; i < 3; i++) {
        gizmoAxes[i]->setGizmoColor(static_cast<float>(i == 1), static_cast<float>(i == 2), static_cast<float>(i == 0), 1.f);
        gizmoAxes[i]->setVisibility(true);
        gizmoAxes[i]->setName(gizmoNames[i]);
        gizmoAxes[i]->setAllowDragSelect(true);

        if (tMode != transMode::rotate_axis) {
            gizmoAxes[i]->m_nameFlag |= (1ULL << (m_axisBitFlagOffset[toType(tMode)] + m_axisBitFlagMap[i]));
        } else {
            gizmoAxes[i]->m_nameFlag |= (1ULL << (m_axisBitFlagOffset[toType(tMode)] + i));
        }

        if (tMode != transMode::rotate && tMode != transMode::rotate_axis && tMode != transMode::passive) {
            gizmoPlanes[i]->m_nameFlag |= (1ULL << (m_axisBitFlagOffset[toType(tMode)] + m_planeBitFlagMap[i * 2]));
            gizmoPlanes[i]->m_nameFlag |= (1ULL << (m_axisBitFlagOffset[toType(tMode)] + m_planeBitFlagMap[i * 2 + 1]));
            gizmoPlanes[i]->setAllowDragSelect(true);
        }

        // rotate gizmo axes into correct orientation

        // translation gizmo 2d plane handler colors
        vector upperPlaneCol = {vec3{0.f, 1.f, 0.f}, vec3{0.f, 0.f, 1.f}, vec3{0.f, 1.f, 0.f}};
        vector lowerPlaneCol = {vec3{0.f, 0.f, 1.f}, vec3{1.f, 0.f, 0.f}, vec3{1.f, 0.f, 0.f}};

        // rotation gizmo axes orientations
        vector vr = {vec4{static_cast<float>(M_PI) * -0.5f, 0.f, 0.f, 1.f}, vec4{static_cast<float>(M_PI) * -0.5f, 0.f, 1.f, 0.f},
                           vec4{static_cast<float>(M_PI) * 0.5f, 1.f, 0.f, 0.f}};

        // translation gizmo axes orientations
        vector vtr = {vec4{static_cast<float>(M_PI) * 0.5f, 1.f, 0.f, 0.f}, vec4{static_cast<float>(M_PI) * -0.5f, 0.f, 0.f, 1.f}};

        // translation gizmo 2d plane handlers orientation
        vector vpr = {vec4{static_cast<float>(M_PI) * -0.5f, 0.f, 1.f, 0.f}, vec4{static_cast<float>(M_PI) * 0.5f, 1.f, 0.f, 0.f}};

        if (tMode != transMode::rotate && tMode != transMode::rotate_axis) {
            if (i != 2) {
                gizmoAxes[i]->rotate(vtr[i].x, vtr[i].y, vtr[i].z, vtr[i].w);
                if (tMode == transMode::translate) gizmoPlanes[i]->rotate(vpr[i].x, vpr[i].y, vpr[i].z, vpr[i].w);
            }

            if (tMode == transMode::translate) {
                gizmoPlanes[i]->setGizmoUpperColor(upperPlaneCol[i].x, upperPlaneCol[i].y, upperPlaneCol[i].z,
                                                   planeAlpha);
                gizmoPlanes[i]->setGizmoLowerColor(lowerPlaneCol[i].x, lowerPlaneCol[i].y, lowerPlaneCol[i].z,
                                                   planeAlpha);
            }
        } else {
            gizmoAxes[i]->rotate(vr[i].x, vr[i].y, vr[i].z, vr[i].w);
        }
    }
}

void SNGizmo::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo) {}

void SNGizmo::selectAxis(size_t idx) {
    if (idx < 3) {
        for (uint64_t i = 0; i < 3; i++) {
            if (gizmoAxes[i]->m_nameFlag == (1ULL << (m_axisBitFlagOffset[toType(tMode)] + idx))) {
                gizmoAxes[i]->setSelected(true);
                m_selectedAxis = static_cast<int>(idx);
                break;
            }
        }
    } else if (tMode == transMode::translate) {
        for (uint64_t i = 0; i < 3; i++) {
            if (tMode != transMode::rotate && tMode != transMode::rotate_axis && tMode != transMode::passive) {
                std::array<uint64_t, 3> m{1, 2, 0};
                uint64_t                pidx = m[idx - 3];
                uint64_t name = (1ULL << (m_axisBitFlagOffset[toType(tMode)] + m_planeBitFlagMap[pidx * 2]));
                name |= (1ULL << (m_axisBitFlagOffset[toType(tMode)] + m_planeBitFlagMap[pidx * 2 + 1]));

                if (gizmoPlanes[i]->m_nameFlag == name) {
                    gizmoPlanes[i]->setSelected(true);
                    m_selectedAxis = static_cast<int>(idx);
                    break;
                }
            }
        }
    }
}

void SNGizmo::selectNextAxis() {
    for (const auto& it : gizmoAxes) {
        it->setSelected(false);
    }

    for (const auto& it : gizmoPlanes) {
        it->setSelected(false);
    }

    selectAxis(tMode == transMode::translate ? (m_selectedAxis + 1) % 6 : (m_selectedAxis + 1) % 3);
}

void SNGizmo::selectPrevAxis() {
    for (const auto& it : gizmoAxes) {
        it->setSelected(false);
    }

    for (const auto& it : gizmoPlanes) {
        it->setSelected(false);
    }

    selectAxis(tMode == transMode::translate ? (m_selectedAxis - 1 + 6) % 6 : (m_selectedAxis - 1 + 3) % 3);
}

}  // namespace ara
