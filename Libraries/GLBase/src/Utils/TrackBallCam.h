//
// Created by user on 14.09.2021.
//

#pragma once

#include <AnimVal.h>
#include <StopWatch.h>
#include <Utils/Camera.h>

namespace ara {

class TbModData {
public:
    glm::vec3    *trans     = nullptr;
    glm::quat    *rotQ      = nullptr;
    glm::vec3    *rotEuler  = nullptr;
    glm::vec3    *mouseRot  = nullptr;
    bool          dragStart = false;
    bool          fadeStart = false;
    mouseDragType transType = mouseDragType::none;
    int           mode      = 0;
};

class TrackBallCam : public Camera {
public:
    enum class mode : int { none = 0, fixAxisMapping, firstPerson, roll, orbitNoRoll, orbit, arcBall, arcBallShoe };
    enum class snap : int { x_pos = 0, y_pos, z_pos, x_neg, y_neg, z_neg };
    glm::vec3 m_axes[6]{glm::vec3{1.f, 0.f, 0.f},  glm::vec3{0.f, 1.f, 0.f},  glm::vec3{0.f, 0.f, 1.f},
                        glm::vec3{-1.f, 0.f, 0.f}, glm::vec3{0.f, -1.f, 0.f}, glm::vec3{0.f, 0.f, -1.f}};
    enum class CameraMotionType : int { none = 0, arc, firstperson, pan, roll, zoom };
    enum class RotateMapping : int { none = 0, rightDrag, leftDrag, leftOrRightDrag };
    enum class TransMapping : int { none = 0, shiftRightDrag, leftOrShiftRightDrag };
    enum class RollMapping : int { none = 0, shiftCtrlRightDrag };
    enum class WheelMapping : int { none = 0, zoom };
    enum class ZoomMapping : int { none = 0, ctrlRightDrag, leftOrCtrlRightDrag };
    TrackBallCam() : Camera() { m_camera = reinterpret_cast<Camera *>(this); }

    TrackBallCam(camType setup, float screenWidth, float screenHeight, float left, float right, float bottom, float top,
                 float cpX = 0.f, float cpY = 0.f, float cpZ = 1.f, float laX = 0.f, float laY = 0.f, float laZ = 0.f,
                 float upX = 0.f, float upY = 1.f, float upZ = 0.f, float inNear = 1.f, float inFar = 1000.f,
                 float fov = 45.f)
        : Camera(setup, screenWidth, screenHeight, left, right, bottom, top, cpX, cpY, cpZ, laX, laY, laZ, upX, upY,
                 upZ, inNear, inFar, fov) {
        m_camera = reinterpret_cast<Camera *>(this);
    }

    void       setInteractionStart();
    void       updateMatrices();
    void       updateSuperClass(bool callUpdtCb);
    void       updateFromExternal(bool forceUpdt = false);  // used in calibrator Stage3DWin
    void       dragOrbit();
    void       dragOrbitNoRoll();
    void       dragFixMouseAxisMapping();
    void       dragFirstPerson();
    void       dragRoll(float offset);
    void       dragArcBall();
    void       dragArcBallShoe();
    void       computeRotationBetweenVectors(const glm::vec3 &u, const glm::vec3 &v, glm::quat &result) const;
    static void       computePointOnSphere(const glm::vec2 &point, glm::vec3 &result);
    glm::quat  screen_to_arcball(const glm::vec2 &p);
    void       snapToAxis(snap axis);
    void       fadeTo(double duration);
    void       fadeTo(glm::vec3 &dstEuler, glm::vec3 &dstTrans, double duration);
    void       modelMatToTrackBallCam(bool updtEuler = true, bool updtPos = true);
    glm::mat4 &lookAtBlend(float p, glm::vec3 *euler = nullptr, glm::vec3 *trans = nullptr);

    // key interaction
    void keyDown(int keyCode, bool shiftPressed, bool altPressed, bool ctrlPressed);
    void keyUp(int keyCode, bool shiftPressed, bool altPressed, bool ctrlPressed);

    // mouse interaction
    void mouseDrag(float x, float y, bool shiftPressed, bool altPressed, bool ctrlPressed, glm::vec2 &rotScale);
    void mouseDownLeft(float x,
                       float y);         ///> in normalized coordinates [0,1] relative
                                         /// to view, [0,0] is left/top
    void mouseUpLeft(float x, float y);  ///> in normalized coordinates [0,1]
                                         /// relative to view, [0,0] is left/top
    void mouseDownMiddle(float x, float y) {
        m_mbState[(int)mouseButt::middle].pressed = true;
    }  ///> in normalized coordinates [0,1] relative to view, [0,0] is left/top
    void mouseUpMiddle(float x, float y) {
        m_mbState[(int)mouseButt::middle].pressed = false;
    }  ///> in normalized coordinates [0,1] relative to view, [0,0] is left/top
    void mouseDownRight(float x,
                        float y);  ///> in normalized coordinates [0,1] relative
                                   /// to view, [0,0] is left/top
    void mouseUpRight(float x,
                      float y);  ///> in normalized coordinates [0,1]
                                 /// relative to view, [0,0] is left/top
    void mouseWheel(float offset);
    bool updateExt(glm::vec3 *trans, glm::vec3 *rot);

    void updateFade() {
        m_animRot.update();
        m_animTrans.update();
        m_animTransf.update();
    }

    bool isAnimating() { return !m_animTrans.stopped() || !m_animTransf.stopped(); }
    bool fadeStopped() { return m_animTrans.stopped(); }
    bool fadeTransfStopped() { return m_animTransf.stopped(); }
    void updateRotQ() { m_dcOri = glm::quat(glm::eulerAngleYXZ(m_tbEulerAngle.y, m_tbEulerAngle.x, m_tbEulerAngle.z)); }

    /** Get the actual Trackball translation (absolute in world coordinates) */
    glm::vec3 &getTrackBallTrans() { return m_tbTrans; }

    /** Get the actual Trackball rotation (in radian euler angles) */
    glm::vec3 &getTrackBallRot() { return m_tbEulerAngle; }

    /** directly set the trackball translation, without any matrix updated (used
     * for camera swapping) */
    void setTrackBallTrans(glm::vec3 *v) { m_tbTrans = *v; }
    void setTrackBallTrans(glm::vec3 &&v) { m_tbTrans = v; }

    /** directly set the trackball rotation without any matrix updated (used for
     * camera swapping) */
    void setTrackBallRot(glm::vec3 *r) {
        m_tbEulerAngle = *r;
        updateRotQ();
    }

    void setTrackBallRot(glm::vec3 &&r) {
        m_tbEulerAngle = r;
        updateRotQ();
    }

    void       setVirtRotCenter(glm::vec3 *r) { m_virtRotCenter = *r; }
    void       setVirtRotCenter(glm::vec3 &&r) { m_virtRotCenter = r; }
    void       setSnapAxisRotCenter(glm::vec3 *r) { m_snapAxisRotCenter = *r; }
    void       setSnapAxisRotCenter(glm::vec3 &&r) { m_snapAxisRotCenter = r; }
    glm::vec3 &getSnapAxisRotCenter() { return m_snapAxisRotCenter; }
    void       setTbMouseRot(glm::vec3 *r) { m_tbMouseRot = *r; }
    void       setLookAtBlendSrcPos(glm::vec3 *v) { m_srcCamPos = *v; }
    void       setLookAtBlendSrcPos(glm::vec3 &&v) { m_srcCamPos = v; }
    void       setLookAtBlendSrcUpVec(glm::vec3 *v) { m_srcUpVec = *v; }
    void       setLookAtBlendSrcUpVec(glm::vec3 &&v) { m_srcUpVec = v; }
    void       setLookAtBlendSrcLookAt(glm::vec3 *v) { m_srcLookAt = *v; }
    void       setLookAtBlendSrcLookAt(glm::vec3 &&v) { m_srcLookAt = v; }
    void       setLookAtBlendDstPos(glm::vec3 *v) { m_dstCamPos = *v; }
    void       setLookAtBlendDstPos(glm::vec3 &&v) { m_dstCamPos = v; }
    void       setLookAtBlendDstUpVec(glm::vec3 *v) { m_dstUpVec = *v; }
    void       setLookAtBlendDstUpVec(glm::vec3 &&v) { m_dstUpVec = v; }
    void       setLookAtBlendDstLookAt(glm::vec3 *v) { m_dstLookAt = *v; }
    void       setLookAtBlendDstLookAt(glm::vec3 &&v) { m_dstLookAt = v; }

    void setTrackBallMode(TrackBallCam::mode mode) {
        m_mode = mode;
        if (mode == mode::fixAxisMapping) m_virtRotCenter = glm::vec3(0.f, 0.f, -3.f);
    }

    void setRotateInteraction(RotateMapping mapping) { m_rotateMapping = mapping; }
    void setRollInteraction(RollMapping mapping) { m_rollMapping = mapping; }
    void setTransInteraction(TransMapping mapping) { m_transMapping = mapping; }
    void setWheelInteraction(WheelMapping mapping) { m_wheelMapping = mapping; }
    void setZoomInteraction(ZoomMapping mapping) { m_zoomMapping = mapping; }
    void setUptCamSceneNodeCb(std::function<void(TbModData &)> f) { m_updtSceneNodeCb = std::move(f); }
    void removeUpdtCamSceneNodeCb() { m_updtSceneNodeCb = nullptr; }
    void addTrackBallUpdtCb(void *name, std::function<void(TbModData &)> f) { m_trackBallUpdtCb[name] = std::move(f); }
    void removeTrackBallUpdtCb(void *name) { m_trackBallUpdtCb[name] = nullptr; }
    void setCamSetUpdtCb(std::function<void()> f) { m_camSetUpdtCb = std::move(f); }
    std::function<void()> *getCamSetUpdtCb() { return &m_camSetUpdtCb; }
    void                   removeCamSetUpdtCb() { m_camSetUpdtCb = nullptr; }
    Camera                *getCamera() { return m_camera; }
    void                   setCamera(Camera *c) { m_camera = c; }
    TrackBallCam::mode     getMode() const { return m_mode; }
    bool                   useTrackBall() const { return m_useTrackBall; }
    void                   setUseTrackBall(bool val) { m_useTrackBall = val; }
    AnimVal<glm::vec3>    &getAnimRot() { return m_animRot; }
    AnimVal<float>        &getAnimTrans() { return m_animTransf; }
    double                &getAnimDur() { return m_animDur; }
    void                   setMouseRotExp(float exp) { m_mouseRotExp = exp; }
    glm::vec3             &getLookAtBlendSrcCamPos() { return m_srcCamPos; }
    glm::vec3             &getLookAtBlendDstCamPos() { return m_dstCamPos; }
    glm::vec3             &getLookAtBlendDstUpVec() { return m_dstUpVec; }

private:
    glm::vec3 m_tbMouseDownPos{0.f};
    glm::vec3 m_tbTrans{0.f};  ///< absolute trackball camera position in world
                               ///< coordinate system
    glm::vec3 m_tbMouseTrans{0.f};
    glm::vec3 m_tbMouseDownEulerAngle{0.f};
    glm::vec3 m_tbEulerAngle{0.f};  ///< absolute trackball camera rotation in
                                    ///< world coordinate system
    glm::vec3 m_tbMouseRot{0.f};
    glm::vec3 m_tbMouseRotScaled{0.f};
    glm::mat4 m_tbMouseDownCamModelMat = glm::mat4{1.f};
    glm::mat4 m_tbResultModelMat       = glm::mat4{1.f};
    glm::mat4 m_blendMat               = glm::mat4{1.f};
    glm::vec3 m_lookAt{0.f};
    glm::vec3 m_camPos{0.f};
    glm::vec3 m_virtRotCenter{0.f, 0.f, 0.f};
    glm::vec3 m_transVCamCenter{0.f, 0.f, 0.f};
    glm::vec3 m_mdCamTrans{0.f};
    glm::vec3 m_sign{0.f};
    glm::vec3 m_arcBallStartVec{0.f};
    glm::vec3 m_arcBallStopVec{0.f};
    glm::vec3 m_snapAxisRotCenter{0.f};
    glm::vec3 m_srcUpVec{0.f};
    glm::vec3 m_srcCamPos{0.f};
    glm::vec3 m_srcLookAt{0.f};
    glm::vec3 m_dstUpVec{0.f};
    glm::vec3 m_dstCamPos{0.f};
    glm::vec3 m_dstLookAt{0.f};
    glm::vec3 m_blendUpVec{0.f};
    glm::vec3 m_blendCamPos{0.f};
    glm::vec3 m_blendLookAt{0.f};
    glm::vec4 m_camUpVec{0.f, 1.f, 0.f, 0.f};
    glm::vec4 m_dirVec{0.f};
    glm::vec2 m_actMouseCoord{0.f};
    glm::vec2 m_mouseDownCoord{0.f};
    glm::vec2 m_mouseDragOffs{0.f};
    glm::vec2 m_radians{0.f};
    glm::vec2 m_sta_proj{0.f};
    glm::vec2 m_p{0.f};

    glm::quat m_arcBallRot;
    glm::quat m_tbRotQuat;
    glm::quat m_cur_ball;
    glm::quat m_prev_ball;
    glm::quat m_dcOri;

    glm::vec3            m_rotatedAxes[6];
    std::map<float, int> m_sortedAxes;
    Camera              *m_camera = nullptr;  ///> typically this should pointer to the class
                                              /// instance itself. But optionally this can
                                              /// point to an external Camera
    TrackBallCam::mode m_mode = mode::firstPerson;

    RotateMapping m_rotateMapping = RotateMapping::rightDrag;
    TransMapping  m_transMapping  = TransMapping::shiftRightDrag;
    WheelMapping  m_wheelMapping  = WheelMapping::zoom;
    ZoomMapping   m_zoomMapping   = ZoomMapping::ctrlRightDrag;
    RollMapping   m_rollMapping   = RollMapping::shiftCtrlRightDrag;

    std::unordered_map<void *, std::function<void(TbModData &)>> m_trackBallUpdtCb;
    std::function<void(TbModData &)>                             m_updtSceneNodeCb;
    std::function<void()>                                        m_camSetUpdtCb;
    TbModData                                                    m_cbModData;
    mouseDragType                                                m_transType = mouseDragType::none;
    AnimVal<glm::vec3>                                           m_animRot;
    AnimVal<glm::vec3>                                           m_animTrans;
    AnimVal<float>                                               m_animTransf;

    StopWatch m_watch;

    std::array<MouseButtState, 3> m_mbState;

    bool m_shiftPressed = false;
    bool m_altPressed   = false;
    bool m_ctrlPressed  = false;

    bool m_isMouseTranslating = false;
    bool m_forceNewDragStart  = false;
    bool m_useTrackBall       = false;
    bool m_snaFadeStart       = false;
    bool m_eulerValid         = false;

    float  m_mouseRotExp  = 2.0f;
    float  m_arcBallSpeed = 0.5f;
    double m_animDur      = 0.3;

    float m_sta_dist = 0.f;

    static inline glm::vec3 m_zAxis{0.f, 0.f, 1.f};
    /// local variables made members for performance reasons
    glm::vec3 t_newEuler{0.f};
    glm::mat4 t_inv{0.f};
};

}  // namespace ara
