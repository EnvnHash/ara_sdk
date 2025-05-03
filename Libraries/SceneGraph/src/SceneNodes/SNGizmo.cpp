#include "SceneNodes/SNGizmo.h"

#include "SNGizmoRotAxisLetter.h"
#include "CameraSets/CameraSet.h"
#include "SceneNodes/SNGizmoPlane.h"
#include "SceneNodes/SNGizmoRotAxis.h"
#include "SceneNodes/SNGizmoScaleAxis.h"
#include "SceneNodes/SNGizmoTransAxis.h"

using namespace glm;
using namespace std;

namespace ara {

SNGizmo::SNGizmo(transMode _mode, sceneData* sd)
    : SceneNode(sd), m_tMode(_mode), m_gizmoScreenSize(0.15f), m_planeAlpha(0.8f) {
    m_staticNDCSize = 75.f;  // in pixels
                             // m_staticNDCSize = 0.15f;
    m_depthTest     = true;
    m_cullFace      = true;
    m_nodeType      = sceneNodeType::gizmo;
    m_emptyDrawFunc = true;

    setName(getTypeName<SNGizmo>() + "_" + string(nameof::nameof_enum(m_tMode)));
    string  gizmoPlaneNames[3] = {getTypeName<SNGizmo>() + "_Y_Z", getTypeName<SNGizmo>() + "_X_Z",
                                  getTypeName<SNGizmo>() + "_X_Y"};
    twPlane gizmoPlaneTypes[3] = {twPlane::yz, twPlane::xz, twPlane::xy};

    // init Gizmo Planes
    if (m_tMode != transMode::rotate && m_tMode != transMode::rotate_axis && m_tMode != transMode::passive) {
        for (uint i = 0; i < 3; i++) {
            m_gizmoPlanes.emplace_back(dynamic_cast<SNGizmoPlane *>(SceneNode::addChild(make_unique<SNGizmoPlane>())));
            m_gizmoPlanes.back()->setVisibility(true);
            m_gizmoPlanes.back()->setName(gizmoPlaneNames[i]);
            m_gizmoPlanes.back()->setPlaneType(gizmoPlaneTypes[i]);
            m_gizmoPlanes.back()->setAllowDragSelect(true);
        }
    }

    switch (m_tMode) {
        case transMode::translate:
            for (uint i = 0; i < 3; i++) m_gizmoAxes.push_back(dynamic_cast<SNGizmoAxis *>(SceneNode::addChild(make_unique<SNGizmoTransAxis>())));
            m_nameFlag |= GLSG_TRANS_GIZMO;
            break;

        case transMode::passive:
            for (uint i = 0; i < 3; i++) {
                m_gizmoAxes.push_back(dynamic_cast<SNGizmoAxis *>(SceneNode::addChild(make_unique<SNGizmoTransAxis>())));
                m_gizmoAxes[i]->m_selectable = false;
            }
            m_nameFlag |= GLSG_PASSIVE_GIZMO;
            m_staticNDCSize = 0.f;
            break;

        case transMode::rotate:
            for (uint i = 0; i < 3; i++) {
                m_gizmoAxes.push_back(dynamic_cast<SNGizmoAxis*>(SceneNode::addChild(make_unique<SNGizmoRotAxis>())));
            }
            m_nameFlag |= GLSG_ROT_GIZMO;
            break;

        case transMode::rotate_axis:
            for (uint i = 0; i < 3; i++) {
                m_gizmoAxes.push_back(dynamic_cast<SNGizmoAxis *>(SceneNode::addChild(make_unique<SNGizmoRotAxisLetter>())));
            }
            m_nameFlag |= GLSG_ROT_AXIS_GIZMO;
            break;

        case transMode::scale:
            for (uint i = 0; i < 3; i++) {
                m_gizmoAxes.push_back(dynamic_cast<SNGizmoAxis *>(SceneNode::addChild(make_unique<SNGizmoScaleAxis>())));
            }
            m_nameFlag |= GLSG_SCALE_GIZMO;
            break;
        case transMode::none: break;
        default: break;
    }

    string gizmoNames[3] = {"Gizmo_Z", "Gizmo_X", "Gizmo_Y"};

    for (uint i = 0; i < 3; i++) {
        m_gizmoAxes[i]->setGizmoColor(static_cast<float>(i == 1), static_cast<float>(i == 2), static_cast<float>(i == 0), 1.f);
        m_gizmoAxes[i]->setVisibility(true);
        m_gizmoAxes[i]->setName(gizmoNames[i]);
        m_gizmoAxes[i]->setAllowDragSelect(true);

        if (m_tMode != transMode::rotate_axis) {
            m_gizmoAxes[i]->m_nameFlag |= (1ULL << (m_axisBitFlagOffset[toType(m_tMode)] + m_axisBitFlagMap[i]));
        } else {
            m_gizmoAxes[i]->m_nameFlag |= (1ULL << (m_axisBitFlagOffset[toType(m_tMode)] + i));
        }

        if (m_tMode != transMode::rotate && m_tMode != transMode::rotate_axis && m_tMode != transMode::passive) {
            m_gizmoPlanes[i]->m_nameFlag |= (1ULL << (m_axisBitFlagOffset[toType(m_tMode)] + m_planeBitFlagMap[i * 2]));
            m_gizmoPlanes[i]->m_nameFlag |= (1ULL << (m_axisBitFlagOffset[toType(m_tMode)] + m_planeBitFlagMap[i * 2 + 1]));
            m_gizmoPlanes[i]->setAllowDragSelect(true);
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

        if (m_tMode != transMode::rotate && m_tMode != transMode::rotate_axis) {
            if (i != 2) {
                m_gizmoAxes[i]->rotate(vtr[i].x, vtr[i].y, vtr[i].z, vtr[i].w);
                if (m_tMode == transMode::translate) {
                    m_gizmoPlanes[i]->rotate(vpr[i].x, vpr[i].y, vpr[i].z, vpr[i].w);
                }
            }

            if (m_tMode == transMode::translate) {
                m_gizmoPlanes[i]->setGizmoUpperColor(upperPlaneCol[i].x, upperPlaneCol[i].y, upperPlaneCol[i].z,
                                                   m_planeAlpha);
                m_gizmoPlanes[i]->setGizmoLowerColor(lowerPlaneCol[i].x, lowerPlaneCol[i].y, lowerPlaneCol[i].z,
                                                   m_planeAlpha);
            }
        } else {
            m_gizmoAxes[i]->rotate(vr[i].x, vr[i].y, vr[i].z, vr[i].w);
        }
    }
}

void SNGizmo::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo) {}

void SNGizmo::selectAxis(size_t idx) {
    if (idx < 3) {
        for (uint64_t i = 0; i < 3; i++) {
            if (m_gizmoAxes[i]->m_nameFlag == (1ULL << (m_axisBitFlagOffset[toType(m_tMode)] + idx))) {
                m_gizmoAxes[i]->setSelected(true);
                m_selectedAxis = static_cast<int>(idx);
                break;
            }
        }
    } else if (m_tMode == transMode::translate) {
        for (uint64_t i = 0; i < 3; i++) {
            if (m_tMode != transMode::rotate
                && m_tMode != transMode::rotate_axis
                && m_tMode != transMode::passive) {
                std::array<uint64_t, 3> m{1, 2, 0};
                uint64_t                pidx = m[idx - 3];
                uint64_t name = (1ULL << (m_axisBitFlagOffset[toType(m_tMode)] + m_planeBitFlagMap[pidx * 2]));
                name |= (1ULL << (m_axisBitFlagOffset[toType(m_tMode)] + m_planeBitFlagMap[pidx * 2 + 1]));

                if (m_gizmoPlanes[i]->m_nameFlag == name) {
                    m_gizmoPlanes[i]->setSelected(true);
                    m_selectedAxis = static_cast<int>(idx);
                    break;
                }
            }
        }
    }
}

void SNGizmo::selectNextAxis() {
    for (const auto& it : m_gizmoAxes) {
        it->setSelected(false);
    }

    for (const auto& it : m_gizmoPlanes) {
        it->setSelected(false);
    }

    selectAxis(m_tMode == transMode::translate ? (m_selectedAxis + 1) % 6 : (m_selectedAxis + 1) % 3);
}

void SNGizmo::selectPrevAxis() {
    for (const auto& it : m_gizmoAxes) {
        it->setSelected(false);
    }

    for (const auto& it : m_gizmoPlanes) {
        it->setSelected(false);
    }

    selectAxis(m_tMode == transMode::translate ? (m_selectedAxis - 1 + 6) % 6 : (m_selectedAxis - 1 + 3) % 3);
}

}  // namespace ara
