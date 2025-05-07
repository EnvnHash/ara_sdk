//
// Created by user on 13.01.2021.
//

#include <UIElements/Gizmo.h>
#include <UIElements/GizmoAxisLabel.h>
#include <UIWindow.h>
#include <Utils/FBO.h>
#include <Utils/PingPongFbo.h>
#include <DrawManagers/DrawManagerGizmo.h>

using namespace std;
using namespace glm;

namespace ara {

GizmoAxisLabel::GizmoAxisLabel() : Image() {
    setName(getTypeName<GizmoAxisLabel>());
    m_drawImmediate = true;
}

GizmoAxisLabel::GizmoAxisLabel(const std::string& styleClass) : Image(styleClass) {
    setName(getTypeName<GizmoAxisLabel>());
    m_drawImmediate = true;
}

GizmoAxisLabel::GizmoAxisLabel(const std::string& file, int mipMapLevel, bool keep_aspect, align ax, valign ay)
    : Image(file, mipMapLevel, keep_aspect, ax, ay) {
    setName(getTypeName<GizmoAxisLabel>());
    m_drawImmediate = true;
}

void GizmoAxisLabel::init() {
    m_hasDepth       = true;
    m_canReceiveDrag = true;

    setSepBlendFunc(true);
    setSrcBlendFunc(GL_SRC_ALPHA);
    setDstBlendFunc(GL_ONE_MINUS_SRC_ALPHA);
    setSrcBlendAlphaFunc(GL_ONE);
    setDstBlendAlphaFunc(GL_ONE);
}

bool GizmoAxisLabel::draw(uint32_t* objId) {
    if (m_hasLabel) Image::draw(objId);
    updateCamFade();

    return true;  // count up objId
}

void GizmoAxisLabel::pushVaoUpdtOffsets() {
    Div::pushVaoUpdtOffsets();

    if (m_hasLabel && m_imgDB.drawSet) {
        m_imgDB.drawSet->updtNodes.emplace_back(m_imgDB.getUpdtPair());
    }
}

bool GizmoAxisLabel::updateCamFade() {
    if (m_gizmo->getCamera() && !m_gizmo->getCamera()->fadeTransfStopped()) {
        m_gizmo->getCamera()->updateFade();
        setDrawFlag();
        m_gizmo->getSharedRes()->requestRedraw = true;
        return true;
    } else
        return false;
}

void GizmoAxisLabel::mouseIn(hidData* data) {
    Image::mouseIn(data);

    // since this is rendered out of tree, manually call updateDrawData
    if (m_gizmo) {
        m_gizmo->m_updtDrawData = true;
        setDrawFlag();
    }
}

void GizmoAxisLabel::mouseOut(hidData* data) {
    m_setStyleFunc[state::none][styleInit::x] = [] {};
    m_setStyleFunc[state::none][styleInit::y] = [] {};

    UINode::mouseOut(data);  // call this to update styles

    // since this is rendered out of tree, manually call updateDrawData
    if (m_gizmo) {
        m_gizmo->m_updtDrawData = true;
    }

    // since we can't use the labels inside the usual ui node tree, because of
    // custom drawing with z-pos and Fbos and thus custom sharedRes, the
    // requestRedraw Flag isn't set into ui node trees sharedRes, so do it here
    if (m_gizmo && m_gizmo->getSharedRes()) {
        m_gizmo->getSharedRes()->requestRedraw = true;
    }
}

void GizmoAxisLabel::mouseDrag(hidData* data) {
    if (m_gizmo) {
        m_gizmo->drag(data);
    }
    if (data->hit) {
        data->consumed = true;
    }
}

void GizmoAxisLabel::mouseDown(hidData* data) {
    if (!m_gizmo || data->clickedObjId != m_objIdMin) {
        return;
    }

    m_mousePosRel = m_gizmo->getGizmoRelMousePos(data->mousePos);
    m_gizmo->getCamera()->mouseDownLeft(m_mousePosRel.x, m_mousePosRel.y);

    data->consumed = true;
    m_leftPressed  = true;
}

void GizmoAxisLabel::mouseUp(hidData* data) {
    if (!m_leftPressed || !m_gizmo || data->clickedObjId != m_objIdMin) {
        return;
    }

    if (m_resetExcludeFromStyles) {
        m_gizmo->excludeLabelsFromStyles(false);  // Note: must be called before UINode::mouseUp and setSelected(false, true)
        m_resetExcludeFromStyles = false;
    }

    UINode::mouseUp(data);

    if (!data->dragging) {
        m_gizmo->getCamera()->snapToAxis(m_gizAxis);
    } else {
        if (getWindow()) {
            getWindow()->setMouseCursorVisible(true);

            // reposition the cursor to be exactly over the label
            vec2 newMousePos = m_gizmo->getWinPos() + getWinPos() + getSize() * 0.5f;
#ifdef ARA_USE_GLFW
            runOnMainThread([this, newMousePos] {
                glfwSetCursorPos(getWindow()->getWinHandle()->getCtx(), newMousePos.x * getWindow()->getPixelRatio(),
                                 newMousePos.y * getWindow()->getPixelRatio());
                return true;
            });
#elif _WIN32
            glm::vec2 winOffs;
            if (getWindow()->extGetWinOffs()) winOffs = getWindow()->extGetWinOffs()();

            // in contrast to GLFW this must be relative to the screen not the
            // window
            SetCursorPos((int)winOffs.x + (int)newMousePos.x * getWindow()->getPixelRatio(),
                         (int)winOffs.y + (int)newMousePos.y * getWindow()->getPixelRatio());
#endif
        }

        // if the actual mouse position is not over the label, force a Mouseout
        // call
        if (data->releasedObjId != m_objIdMin && m_gizmo->m_lastMouseHoverData) {
            setSelected(false, true);
        }
    }

    m_mousePosRel = m_gizmo->getGizmoRelMousePos(data->mousePos);

    if (m_gizmo->getCamera()) {
        m_gizmo->getCamera()->mouseUpLeft(m_mousePosRel.x, m_mousePosRel.y);
    }

    setDrawFlag();
    getSharedRes()->requestRedraw = true;
    data->consumed                = true;
    m_leftPressed                 = false;
}

void GizmoAxisLabel::setGizmoParent(Gizmo* gizmo) {
    m_gizmo = gizmo;
    if (m_gizmo) {
        setSharedRes(m_gizmo->getAuxSharedRes());
    }
}

}  // namespace ara