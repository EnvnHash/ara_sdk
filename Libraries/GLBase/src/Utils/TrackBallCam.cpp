//
// Created by user on 14.09.2021.
//

#include <Log.h>
#include <Asset/AssetColor.h>
#include <Asset/AssetColor.h>
#include <Asset/AssetColor.h>
#include <Asset/AssetColor.h>
#include <Utils/TrackBallCam.h>

#include "Camera.h"

using namespace glm;

namespace ara {

// key interaction
void TrackBallCam::keyDown(int keyCode, bool shiftPressed, bool altPressed, bool ctrlPressed) {
    m_shiftPressed = shiftPressed;
    m_altPressed   = altPressed;
    m_ctrlPressed  = ctrlPressed;
}

void TrackBallCam::keyUp(int keyCode, bool shiftPressed, bool altPressed, bool ctrlPressed) {
    m_shiftPressed = shiftPressed;
    m_altPressed   = altPressed;
    m_ctrlPressed  = ctrlPressed;

    if (m_isMouseTranslating && (keyCode == GLSG_KEY_LEFT_SHIFT || keyCode == GLSG_KEY_RIGHT_SHIFT)) {
        m_forceNewDragStart  = true;
        m_isMouseTranslating = false;
        setInteractionStart();
    }
}

// mouse interaction
void TrackBallCam::mouseDrag(float x, float y, bool shiftPressed, bool altPressed, bool ctrlPressed,
                             glm::vec2 &rotScale) {
    m_shiftPressed = shiftPressed;
    m_altPressed   = altPressed;
    m_ctrlPressed  = ctrlPressed;

    // mouse key states: set dragging flag
    for (auto &it : m_mbState) {
        if (it.pressed) {
            it.dragging = true;
        }
    }

    bool updateMats = false;

    if (m_forceNewDragStart) {
        m_mouseDownCoord.x  = x;
        m_mouseDownCoord.y  = y;
        m_forceNewDragStart = false;
    }

    m_actMouseCoord.x = x;
    m_actMouseCoord.y = y;
    m_mouseDragOffs   = m_actMouseCoord - m_mouseDownCoord;

    // Translation
    if (((m_transMapping == TransMapping::shiftRightDrag || m_transMapping == TransMapping::leftOrShiftRightDrag) &&
         m_mbState[toType(mouseButt::right)].pressed && m_shiftPressed && !m_altPressed && !m_ctrlPressed) ||
        (m_transMapping == TransMapping::leftOrShiftRightDrag && m_mbState[toType(mouseButt::left)].pressed &&
         !m_shiftPressed && !m_altPressed && !m_ctrlPressed)) {
        m_transType      = mouseDragType::translating;
        m_tbMouseTrans.x = m_mouseDragOffs.x * 4.f;
        m_tbMouseTrans.y = -m_mouseDragOffs.y * 4.f;  // ui screen coords to 3D coords (top/left) to
                                                      // center, y must be flipped
        m_isMouseTranslating = true;
        updateMats           = true;
    }

    // Rotation
    if (((m_rotateMapping == RotateMapping::rightDrag && m_mbState[toType(mouseButt::right)].pressed) ||
         (m_rotateMapping == RotateMapping::leftOrRightDrag &&
          (m_mbState[toType(mouseButt::left)].pressed || m_mbState[toType(mouseButt::right)].pressed)) ||
         (m_rotateMapping == RotateMapping::leftDrag && m_mbState[toType(mouseButt::left)].pressed)) &&
        !m_shiftPressed && !m_altPressed && !m_ctrlPressed) {

        m_transType = mouseDragType::rotating;
        for (int i = 0; i < 2; i++) {
            m_radians[i] = m_mouseDragOffs[i] * rotScale[i];
            if (std::isfinite(m_radians[i]) || !std::isnan(m_radians[i])) {
                m_tbMouseRot[(i + 1) % 2] = m_radians[i];
                updateMats                = true;
            }
        }
    }

    // Rolling (rotate around z-axis)
    if (m_rollMapping == RollMapping::shiftCtrlRightDrag
        && m_mbState[toType(mouseButt::right)].pressed
        && m_shiftPressed
        && !m_altPressed
        && m_ctrlPressed) {
        m_transType = mouseDragType::rolling;
        dragRoll(m_mouseDragOffs.y);  // implicitly calls updateMatrices
    }

    // Zooming
    if (!m_shiftPressed && !m_altPressed &&
        (((m_zoomMapping == ZoomMapping::ctrlRightDrag || m_zoomMapping == ZoomMapping::leftOrCtrlRightDrag) &&
          m_mbState[toType(mouseButt::right)].pressed && m_ctrlPressed) ||
         (m_zoomMapping == ZoomMapping::leftOrCtrlRightDrag && m_mbState[toType(mouseButt::left)].pressed &&
          !m_ctrlPressed))) {
        m_transType      = mouseDragType::zooming;
        m_tbMouseTrans.z = m_mouseDragOffs.y * 6.f;
        updateMats       = true;
    }

    if (updateMats) updateMatrices();

    // mouse button states: proc dragStart
    for (auto &it : m_mbState) {
        if (it.dragStart) {
            it.dragStart = false;
        }
    }
}

void TrackBallCam::mouseDownLeft(float x, float y) {
    if (m_mbState[toType(mouseButt::right)].pressed) {
        return;
    }

    m_mbState[toType(mouseButt::left)].pressed   = true;
    m_mbState[toType(mouseButt::left)].dragStart = true;
    m_mouseDownCoord.x                        = x;
    m_mouseDownCoord.y                        = y;
    m_actMouseCoord                           = m_mouseDownCoord;

    if (m_transMapping == TransMapping::leftOrShiftRightDrag || m_rotateMapping == RotateMapping::leftDrag ||
        m_rotateMapping == RotateMapping::leftOrRightDrag || m_zoomMapping == ZoomMapping::leftOrCtrlRightDrag)
        setInteractionStart();

    if (m_transMapping == TransMapping::leftOrShiftRightDrag && m_isMouseTranslating) m_isMouseTranslating = false;
}

void TrackBallCam::mouseUpLeft(float x, float y) {
    m_mbState[toType(mouseButt::left)].pressed   = false;
    m_mbState[toType(mouseButt::left)].dragging  = false;
    m_mbState[toType(mouseButt::left)].dragStart = false;
    m_transType                               = mouseDragType::none;
}

void TrackBallCam::mouseDownRight(float x, float y) {
    if (m_mbState[toType(mouseButt::left)].pressed) {
        return;
    }

    m_mbState[toType(mouseButt::right)].pressed   = true;
    m_mbState[toType(mouseButt::right)].dragStart = true;
    m_mouseDownCoord.x                         = x;
    m_mouseDownCoord.y                         = y;
    m_actMouseCoord                            = m_mouseDownCoord;

    if (m_rotateMapping == RotateMapping::rightDrag || m_transMapping == TransMapping::shiftRightDrag ||
        m_rotateMapping == RotateMapping::leftOrRightDrag)
        setInteractionStart();

    if ((m_transMapping == TransMapping::shiftRightDrag || m_transMapping == TransMapping::leftOrShiftRightDrag) &&
        m_isMouseTranslating)
        m_isMouseTranslating = false;
}

void TrackBallCam::mouseUpRight(float x, float y) {
    if (m_mbState[toType(mouseButt::left)].pressed) {
        return;
    }

    m_mbState[toType(mouseButt::right)].pressed   = false;
    m_mbState[toType(mouseButt::right)].dragging  = false;
    m_mbState[toType(mouseButt::right)].dragStart = false;
    m_transType                                = mouseDragType::none;
}

void TrackBallCam::mouseWheel(float offset) {
    if (m_wheelMapping == WheelMapping::zoom) {
        m_transType      = mouseDragType::zooming;
        m_tbMouseTrans.z = offset;
        updateMatrices();
    }
}

bool TrackBallCam::updateExt(glm::vec3 *trans, glm::vec3 *rot) {
    bool update = false;

    // check if we got an euler angles
    if (!glm::all(glm::equal(*trans, m_tbTrans))) {
        setTrackBallTrans(trans);
        update = true;
    }

    auto newRot = glm::mod(*rot + vec3((float)M_TWO_PI), vec3((float)M_TWO_PI));
    auto oldRot = glm::mod(m_tbEulerAngle + vec3((float)M_TWO_PI), vec3((float)M_TWO_PI));
    auto diff   = glm::abs(oldRot - newRot);

    if (glm::compAdd(diff) > 1.e-5f) {
        setTrackBallRot(rot);
        update = true;
    }

    if (update) updateFromExternal();

    return update;
}

void TrackBallCam::setInteractionStart() {
    if (!std::isfinite(m_tbTrans.x) || std::isnan(m_tbTrans.x)) {
        LOGE << "TrackBallCam::startTrackBallOffset illegal values";
    }

    m_tbMouseDownPos         = m_tbTrans;
    m_tbMouseDownEulerAngle  = m_tbEulerAngle;
    m_tbMouseTrans           = vec3(0.f, 0.f, 0.f);
    m_tbMouseRot             = vec3(0.f, 0.f, 0.f);
    m_tbMouseDownCamModelMat = getModelMatr();
    m_tbResultModelMat       = getModelMatr();

    if (m_mode == mode::arcBall || m_mode == mode::arcBallShoe) {
        m_tbRotQuat = quat(m_tbMouseRot);
    }

    if (m_mode == mode::arcBall) {
        computePointOnSphere(m_mouseDownCoord, m_arcBallStartVec);
    } else if (m_mode == mode::arcBallShoe) {
        m_cur_ball = screen_to_arcball(m_mouseDownCoord);
    }

    for (int i = 0; i < 3; i++) {
        m_mdCamTrans[i] = m_tbMouseDownCamModelMat[3][i];
    }
}

void TrackBallCam::updateMatrices() {
    if (!m_camera) {
        return;
    }

    switch (m_mode) {
        case mode::fixAxisMapping: dragFixMouseAxisMapping(); break;
        case mode::firstPerson: dragFirstPerson(); break;
        case mode::orbitNoRoll: dragOrbitNoRoll(); break;
        case mode::orbit: dragOrbit(); break;
        case mode::arcBall: dragArcBall(); break;
        case mode::arcBallShoe: dragArcBallShoe(); break;
        default: break;
    }

    updateSuperClass(true);
}

void TrackBallCam::updateSuperClass(bool callUpdtCb) {
    setModelMatr(m_tbResultModelMat);  // does implicitly update Camera::m_mvp

    if (m_camSetUpdtCb) {
        m_camSetUpdtCb();   // calls builds buildCamMatrixArrays
    }

    m_cbModData.trans     = &m_tbTrans;
    m_cbModData.rotQ      = &m_dcOri;
    m_cbModData.rotEuler  = &m_tbEulerAngle;
    m_cbModData.mouseRot  = &m_tbMouseRot;
    m_cbModData.transType = m_transType;
    m_cbModData.mode      = static_cast<int>(m_mode);
    m_cbModData.fadeStart = m_snaFadeStart;

    if (m_updtSceneNodeCb) {
        m_updtSceneNodeCb(m_cbModData);
    }

    // avoid feedback, save the last update values
    if (callUpdtCb) {
        // if the user is starting to drag, using any of the three mouse
        // buttons, set the dragStart flag
        m_cbModData.dragStart = false;
        for (auto &it : m_mbState) {
            m_cbModData.dragStart = m_cbModData.dragStart || it.dragStart;
        }

        for (auto &it : m_trackBallUpdtCb) {
            if (it.second) {
                it.second(m_cbModData);  // model translate, rotate (euler angles)
            }
        }
    }
}

void TrackBallCam::updateFromExternal(bool forceUpdt) {
    m_tbResultModelMat = glm::inverse(glm::translate(m_tbTrans) * eulerAngleYXZ(m_tbEulerAngle.y, m_tbEulerAngle.x, m_tbEulerAngle.z));
    m_tbMouseDownCamModelMat = m_tbResultModelMat;
    updateSuperClass(forceUpdt);
}

void TrackBallCam::dragOrbit() {
    // scale m_tbMouseRot
    vec3 sign{m_tbMouseRot.x >= 0.f ? 1.f : -1.f, m_tbMouseRot.y >= 0.f ? 1.f : -1.f, 1.f};
    m_tbMouseRotScaled = glm::abs(m_tbMouseRot) + 1.f;
    m_tbMouseRotScaled = glm::pow(m_tbMouseRotScaled, vec3{3.f});
    m_tbMouseRotScaled = (m_tbMouseRotScaled - 1.f) * sign;

    // transform m_virtRotCenter into camera space
    m_transVCamCenter = vec3(m_tbMouseDownCamModelMat * glm::vec4(m_virtRotCenter, 1.f));

    // offset by virtCent, rotate, offset back
    m_tbResultModelMat = glm::translate(m_transVCamCenter) *
                         glm::eulerAngleYXZ(m_tbMouseRotScaled.x, m_tbMouseRotScaled.y, 0.f) *
                         // glm::eulerAngleXYZ(m_tbMouseRotScaled.y, m_tbMouseRotScaled.x, 0.f) *
                         glm::translate(-m_transVCamCenter) * m_tbMouseDownCamModelMat;

    // translation is also relative to screen
    m_tbResultModelMat = glm::translate(m_tbMouseTrans) * m_tbResultModelMat;

    modelMatToTrackBallCam();
}

void TrackBallCam::dragOrbitNoRoll() {
    // scale m_tbMouseRot
    m_sign.x = m_tbMouseRot.x >= 0.f ? 1.f : -1.f;
    m_sign.y = m_tbMouseRot.y >= 0.f ? 1.f : -1.f;
    m_sign.z = 1.f;

    m_tbMouseRotScaled = glm::abs(m_tbMouseRot) + 1.f;
    m_tbMouseRotScaled = glm::pow(m_tbMouseRotScaled, vec3{m_mouseRotExp});
    m_tbMouseRotScaled = (m_tbMouseRotScaled - 1.f) * m_sign;

    // transform m_virtRotCenter into camera space
    m_transVCamCenter = vec3(m_tbMouseDownCamModelMat * glm::vec4(m_virtRotCenter, 1.f));

    // offset by virtCent, rotate, offset back
    m_tbResultModelMat = glm::translate(m_transVCamCenter) * glm::rotate(m_tbMouseRotScaled.x, vec3(1.f, 0.f, 0.f)) *
                         glm::rotate(m_tbMouseRotScaled.y, vec3(0.f, 1.f, 0.f)) * glm::translate(-m_transVCamCenter) *
                         m_tbMouseDownCamModelMat;

    // translation is also relative to screen
    m_tbResultModelMat = glm::translate(m_tbMouseTrans) * m_tbResultModelMat;

    modelMatToTrackBallCam();
}

void TrackBallCam::dragFixMouseAxisMapping() {
    // y - rotation goes before the actual mat
    m_tbResultModelMat = m_tbMouseDownCamModelMat * glm::rotate(m_tbMouseRot.y, vec3(0.f, 1.f, 0.f));

    // z offset to the actual view -> take this as the center for rotation
    // around x-axis x - rotation is always relative to the horizontal center of the screen
    m_tbResultModelMat = glm::translate(m_virtRotCenter) * glm::rotate(m_tbMouseRot.x, vec3(1.f, 0.f, 0.f)) *
                         glm::translate(-m_virtRotCenter) * m_tbResultModelMat;

    // translation is also relative to screen
    m_tbResultModelMat = glm::translate(m_tbMouseTrans) * m_tbResultModelMat;

    modelMatToTrackBallCam();
}

void TrackBallCam::dragFirstPerson() {
    // camera rotation - to avoid unwanted rotation around z-axis, do this by
    // camera pos -> s_lookAt
    /*   for (int i=0; i<3; i++)
           m_tbEulerAngle[i] = m_tbMouseDownEulerAngle[i];
      //     m_tbEulerAngle[i] = m_tbMouseDownEulerAngle[i] + m_tbMouseRot[i];

       // calculate the camera's rotation matrix, first rotate around x and y
      axis mat4 rot = eulerAngleXYZ(m_tbEulerAngle.x, m_tbEulerAngle.y, 0.f);

       // then rotate around z-axis, that is the camera's dir vector
       m_dirVec = rot * vec4(0.f, 0.f, -1.f, 0.f);
       rot = glm::rotate(-m_tbEulerAngle.z, glm::vec3(m_dirVec)) * rot;

       // camera translation world relative
       if (m_camera->transWorldRelative())
       {
           m_camPos = m_tbMouseDownPos;
           m_camPos.y -= m_tbMouseTrans.y;
           mat4 yRot = glm::rotate(m_tbEulerAngle.y, vec3(0.f, 1.f, 0.f));
           m_camPos -= vec3(yRot * vec4(m_tbMouseTrans.x, 0.f, m_tbMouseTrans.z,
      0.f));
       }
       else
       {
           // camera translation in camera relative
           m_camPos = m_tbMouseDownPos
                      + vec3(rot * vec4(-m_tbMouseTrans.x, -m_tbMouseTrans.y,
      m_tbMouseTrans.z, 0.f));
       }

       m_tbTrans = m_camPos;
       m_lookAt = m_camPos + vec3(m_dirVec);
       m_camUpVec = rot * vec4(0.f, 1.f, 0.f, 0.f);
       m_tbResultModelMat = glm::lookAt(m_camPos, m_lookAt, vec3(m_camUpVec));*/

    m_tbResultModelMat =
        translate(m_tbMouseTrans) * eulerAngleYXZ(-m_tbMouseRot.y, -m_tbMouseRot.x, 0.f) * m_tbMouseDownCamModelMat;

    modelMatToTrackBallCam(false);
}

void TrackBallCam::dragRoll(float offset) {
    if (m_mode == mode::fixAxisMapping || m_mode == mode::firstPerson) {
        m_tbResultModelMat = glm::rotate(offset, m_zAxis) * m_tbMouseDownCamModelMat;
    } else if (m_mode == mode::orbit || m_mode == mode::orbitNoRoll) {
        float offsSign{offset >= 0.f ? 1.f : -1.f};
        float offsScaled = glm::abs(offset) + 1.f;
        offsScaled       = std::pow(offsScaled, 4.f);
        offsScaled       = (offsScaled - 1.f) * offsSign;

        // offset by virtCent, rotate, offset back
        m_tbResultModelMat = glm::translate(m_virtRotCenter + m_mdCamTrans) *
                             glm::rotate(offsScaled, vec3(0.f, 0.f, 1.f)) *
                             glm::translate(-m_virtRotCenter - m_mdCamTrans) * m_tbMouseDownCamModelMat;
    }

    modelMatToTrackBallCam(true, false);
    updateSuperClass(true);
}

void TrackBallCam::dragArcBall() {
    computePointOnSphere(m_actMouseCoord, m_arcBallStopVec);
    computeRotationBetweenVectors(m_arcBallStartVec, m_arcBallStopVec, m_arcBallRot);

    // Reverse so scene moves with cursor and not away due to camera model.
    m_tbRotQuat = m_arcBallRot * m_tbRotQuat;

    // After applying drag, reset relative start state.
    m_tbResultModelMat = glm::translate(m_tbMouseTrans) * glm::mat4(m_arcBallRot) * m_tbMouseDownCamModelMat;

    modelMatToTrackBallCam();
}

void TrackBallCam::computePointOnSphere(const vec2 &point, vec3 &result) {
    // https://www.opengl.org/wiki/Object_Mouse_Trackball
    float x = -(2.f * point.x - 1.f);
    float y = (2.f * point.y - 1.f);

    float length2 = x * x + y * y;

    if (length2 <= .5) {
        result.z = static_cast<float>(sqrt(1.0 - length2));
    } else {
        result.z = 0.5f / sqrt(length2);
    }

    float norm = 1.0f / sqrt(length2 + result.z * result.z);

    result.x = x * norm;
    result.y = y * norm;
    result.z *= norm;
}

void TrackBallCam::computeRotationBetweenVectors(const vec3 &u, const vec3 &v, quat &result) const {
    float     cosTheta = glm::dot(u, v);
    glm::vec3 rotationAxis{0.f};

    if (cosTheta < -1.0f + glm::epsilon<float>()) {
        // Parallel and opposite directions.
        rotationAxis = cross(glm::vec3(0.f, 0.f, 1.f), u);

        if (glm::length2(rotationAxis) < 0.01) {
            // Still parallel, retry.
            rotationAxis = cross(glm::vec3(1.f, 0.f, 0.f), u);
        }

        rotationAxis = normalize(rotationAxis);
        result       = angleAxis(180.0f, rotationAxis);
    } else if (cosTheta > 1.0f - glm::epsilon<float>()) {
        // Parallel and same direction.
        result = quat(1, 0, 0, 0);
        return;
    } else {
        float theta  = acos(cosTheta);
        rotationAxis = glm::cross(u, v);

        rotationAxis = glm::normalize(rotationAxis);
        result       = glm::angleAxis(theta * m_arcBallSpeed, rotationAxis);
    }
}

void TrackBallCam::dragArcBallShoe() {
    if (m_transType == mouseDragType::rotating) {
        m_prev_ball = screen_to_arcball(m_actMouseCoord);
        m_tbRotQuat = m_prev_ball * m_cur_ball;
    }

    m_tbResultModelMat = glm::translate(m_tbMouseTrans) * glm::mat4(m_tbRotQuat) * m_tbMouseDownCamModelMat;
    modelMatToTrackBallCam();
}

quat TrackBallCam::screen_to_arcball(const vec2 &p) {
    m_p        = vec2(1.f - p.x, p.y) * 2.f - 1.f;
    m_sta_dist = glm::dot(m_p, m_p);

    // If we're on/in the sphere return the point on it
    if (m_sta_dist <= 1.f) {
        return {0.0, m_p.x, m_p.y, std::sqrt(1.f - m_sta_dist)};
    } else {
        // otherwise, we project the point onto the sphere
        m_sta_proj = glm::normalize(m_p);
        return {0.0, m_sta_proj.x, m_sta_proj.y, 0.f};
    }
}

void TrackBallCam::snapToAxis(snap axis) {
    // avoid overlapping fades
    if (!m_animTransf.stopped()) {
        return;
    }

    // do the snapping by calculating a lookAt matrix and decompose this to translation and eulerAngles
    t_inv       = glm::inverse(m_tbMouseDownCamModelMat);
    m_srcUpVec  = vec3(t_inv * vec4{0.f, 1.f, 0.f, 1.f});
    m_srcCamPos = vec3(t_inv * vec4{0.f, 0.f, 1.f, 1.f});

    // check which axis has the greatest y-component, that is, which axis is
    // looking upwards
    m_sortedAxes.clear();
    for (int i = 0; i < 6; i++) {
        m_rotatedAxes[i]                 = vec3(getModelMatr() * vec4(m_axes[i], 1.f));
        m_sortedAxes[m_rotatedAxes[i].y] = i;
    }

    m_dstUpVec = m_axes[(--m_sortedAxes.end())->second];

    vec3 dirVec{axis == snap::x_pos ? 1.f : (axis == snap::x_neg ? -1.f : 0.f),
                axis == snap::y_pos ? 1.f : (axis == snap::y_neg ? -1.f : 0.f),
                axis == snap::z_pos ? 1.f : (axis == snap::z_neg ? -1.f : 0.f)};

    // in case the up vector is part of the selected axis
    if ((dirVec.x != 0.f && std::fabs(dirVec.x) == std::fabs(m_dstUpVec.x)) ||
        (dirVec.y != 0.f && std::fabs(dirVec.y) == std::fabs(m_dstUpVec.y)) ||
        (dirVec.z != 0.f && std::fabs(dirVec.z) == std::fabs(m_dstUpVec.z))) {
        // if the upper part was clicked, take the axis with the smallest
        // z-component, if the lower part was clicked, take the axis with the
        // greatest z-component
        m_sortedAxes.clear();
        vec3 dirVecAbs = glm::abs(dirVec);
        for (int i = 0; i < 6; i++) {
            vec3 axAbs = glm::abs(m_axes[i]);
            vec3 diff = glm::abs(dirVecAbs - axAbs);

            if (glm::compAdd(diff) != 0.f) {
                m_sortedAxes[m_rotatedAxes[i].z] = i;
            }
        }

        if (!m_sortedAxes.empty()) {
            m_dstUpVec = m_axes[vec3(getModelMatr() * vec4(dirVec, 1.f)).y >= 0.f ? m_sortedAxes.begin()->second
                                                                                  : (--m_sortedAxes.end())->second];
        } else {
            return;
        }
    }

    m_dstCamPos = dirVec;
    if (glm::compAdd(glm::abs(m_srcCamPos - m_dstCamPos)) > 0.001f) {
        for (int i = 0; i < 3; i++) {
            m_srcLookAt[i] = 0.f;
            m_dstLookAt[i] = 0.f;
        }
        fadeTo(m_animDur);
    }
}

void TrackBallCam::fadeTo(double duration) {
    m_snaFadeStart = true;

    m_animTransf.start(0.f, 1.f, duration, false, [this](const float &p) {
        lookAtBlend(p, &m_tbEulerAngle);

        m_tbResultModelMat = glm::inverse(glm::eulerAngleYXZ(m_tbEulerAngle.y, m_tbEulerAngle.x, m_tbEulerAngle.z));
        m_dcOri            = glm::quat(m_tbResultModelMat);
        m_transType        = mouseDragType::snapToAxis;

        updateSuperClass(true);

        if (m_snaFadeStart) {
            m_snaFadeStart = false;
        }
    });
}

void TrackBallCam::fadeTo(const vec3 &dstEuler, const vec3 &dstTrans, double duration) {
    m_animRot.start(m_tbEulerAngle, dstEuler, duration, false, [this](const vec3 &r) { m_tbEulerAngle = r; });

    m_animTrans.start(m_tbTrans, dstTrans, duration, false, [this](const vec3 &t) {
        m_tbTrans          = t;
        m_transType        = mouseDragType::none;
        m_tbResultModelMat = inverse(translate(m_tbTrans) *
                                          eulerAngleYXZ(m_tbEulerAngle.y, m_tbEulerAngle.x, m_tbEulerAngle.z));
        updateSuperClass(true);
    });
}

void TrackBallCam::modelMatToTrackBallCam(bool updtEuler, bool updtPos) {
    t_inv = inverse(m_tbResultModelMat);
    decomposeRot(t_inv, m_dcOri);
    m_tbTrans = vec3(t_inv[3]);

    glm::extractEulerAngleYXZ(t_inv, t_newEuler.y, t_newEuler.x, t_newEuler.z);

    m_eulerValid = glm::all(glm::isfinite(t_newEuler)) && !glm::all(glm::isnan(t_newEuler));
    if (!m_eulerValid) {
        LOGE << "TrackBallCam::modelMatToTrackBallCam t_newEuler got in illegal values";
    }

    if (updtEuler && m_eulerValid) {
        m_tbEulerAngle = t_newEuler;
    }
}

glm::mat4 &TrackBallCam::lookAtBlend(float p, glm::vec3 *euler, glm::vec3 *trans) {
    m_blendUpVec  = glm::mix(m_srcUpVec, m_dstUpVec, p);
    m_blendCamPos = glm::mix(m_srcCamPos, m_dstCamPos, p);
    m_blendLookAt = glm::mix(m_srcLookAt, m_dstLookAt, p);
    m_blendMat    = glm::lookAt(m_blendCamPos, m_blendLookAt, m_blendUpVec);

    if (euler || trans) {
        t_inv = inverse(m_blendMat);

        if (euler) {
            extractEulerAngleYXZ(t_inv, t_newEuler[1], t_newEuler[0], t_newEuler[2]);
            std::copy_n(&t_newEuler[0], 3, &(*euler)[0]);
        }

        if (trans) {
            std::copy_n(&t_inv[3][0], 3, &(*trans)[0]);
        }
    }

    return m_blendMat;
}

}  // namespace ara