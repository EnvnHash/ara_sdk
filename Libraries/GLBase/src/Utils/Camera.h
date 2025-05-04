//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
/**
 * \brief Camera, Helper Class for standard OpenGL Camera Setups (projection,
 *frustum, etc...)
 *
 *	used in CameraSets
 *  expects the shader to receive uniform mat4 m_mvp;
 *
 */

#pragma once

#include <GlbCommon/GlbCommon.h>

namespace ara {

enum class camType : int { perspective = 0, frustum, ortho };
enum class camUpt : int { ModelTrans = 0, ModelRot, ModelMat, ViewMat, ProjMat, FrustMult };

struct CameraInitParams {
    camType cTyp{};
    glm::vec2 screenSize{};
    glm::vec4 rect{}; // left, right, bottom, top
    glm::vec3 cp={0.f, 0.f, 1.f};
    glm::vec3 la={0.f, 0.f, 0.f};
    glm::vec3 up = {0.f, 1.f, 0.f};
    float nearLim = 1.f;
    float farLim = 1000.f;
    float fov = 45.f;
};

class Camera {
public:
    Camera() { init(); }
    explicit Camera(const CameraInitParams& params);
    virtual ~Camera() = default;

    void init(bool fromChangedScreenSize = false);
    void buildMatrices(bool fromChangedScreenSize = false);

    void setupPerspective(float fov = 60.f, float nearDist = 0.f, float farDist = 0.f, int scrWidth = 0.f,
                          int scrHeight = 0.f);

    void setScreenSize(uint32_t width, uint32_t height);
    void setModelMatr(const glm::mat4 &modelMatr);
    void setViewMatr(const glm::mat4 &viewMatr);
    void setProjMatr(const glm::mat4 &projMatr);
    void setFrustMult(const float *multVal);
    bool setFishEyeParam();
    void setType(camType t) { m_type = t; }
    void setCamPos(glm::vec3 pos) { m_camPos = pos; }
    void setLookAt(glm::vec3 lookAt) { m_camLookAt = lookAt; }
    void setUpVec(glm::vec3 upVec) { m_camUpVec = upVec; }
    void setCamPos(float x, float y, float z);
    void setLookAt(float x, float y, float z);
    void setClearColor(glm::vec4 col) { m_clearColor = col; }
    void setClearColor(float r, float g, float b, float a) ;
    void setFloorSwitch(float val) { m_floorSwitch = val; }
    void setFov(float val) { m_fov = val; }
    void setUpdtCb(std::function<void(camUpt)> f) { m_updtCb = std::move(f); }
    void setFisheyeOpenAngle(float openAngle);
    void switchFishEye(bool val);
    void setFishEyeAspect(float val);
    void setBorderPix(int val);
    void setScreenWidth(int width) { m_screenSize.x = static_cast<float>(width); }
    void setScreenHeight(int height) { m_screenSize.y = static_cast<float>(height); }
    void setForceUpdtProjMat(bool val) { m_forceUpdtProjMat = val; }
    void setForceUpdtCb(bool val) { m_forceUpdtCb = val; }
    void setFixAspectRatio(float val) { m_fixAspectRatio = val; }
    void setViewport(glm::vec4 &vp) { m_viewport = vp; }
    void setViewport(glm::vec4 &&vp) { m_viewport = vp; }

    void modelTrans(float x, float y, float z);
    void modelRot(float angle, float axisX, float axisY, float axisZ);

    glm::mat4          &getMVP() { return m_mvp; }
    glm::mat4          &getViewMatr() { return m_view; }
    glm::mat4          &getModelMatr() { return m_model; }
    glm::mat4          &getProjectionMatr() { return m_projection; }
    glm::mat3          &getNormalMatr() { return m_normal; }
    GLfloat            *getMVPPtr() { return &m_mvp[0][0]; }
    GLfloat            *getViewMatrPtr() { return &m_view[0][0]; }
    GLfloat            *getModelMatrPtr() { return &m_model[0][0]; }
    GLfloat            *getProjMatrPtr() { return &m_projection[0][0]; }
    GLfloat            *getNormMatrPtr() { return &m_normal[0][0]; }
    glm::vec3          &getCamPos() { return m_camPos; }
    glm::vec3           getViewerVec() { return m_camPos - m_camLookAt; }
    glm::vec3          &getCamLookAt() { return m_camLookAt; }
    glm::vec3          &getCamUp() { return m_camUpVec; }
    glm::vec4          &getClearColor() { return m_clearColor; }
    [[nodiscard]] float getLeft() const { return m_left; }
    [[nodiscard]] float getRight() const { return m_right; }
    [[nodiscard]] float getBottom() const { return m_bottom; }
    [[nodiscard]] float getTop() const { return m_top; }
    [[nodiscard]] float getNear() const { return m_near; }
    [[nodiscard]] float getFar() const { return m_far; }
    [[nodiscard]] float getFov() const { return m_fov; }
    [[nodiscard]] float getFloorSwitch() const { return m_floorSwitch; }
    [[nodiscard]] int   getFishEyeSwitch() const { return m_useFisheye; }
    glm::vec4          &getFishEyeParam() { return m_fishEyeParam0; }
    [[nodiscard]] float getFishEyeAspect() const { return m_feAspect; }
    [[nodiscard]] float getAspect() const { return m_aspectRatio; }
    [[nodiscard]] bool  transWorldRelative() const { return m_transWorldRelative; }
    [[nodiscard]] bool  forceUpdtProjMat() const { return m_forceUpdtProjMat; }
    [[nodiscard]] bool  getForceUpdtCb() const { return m_forceUpdtCb; }
    glm::vec4          &getViewport() { return m_viewport; }

    void debug() const;

    float     m_floorSwitch  = 1.f;
    glm::vec2 m_screenSize{};
    int       m_borderPix    = 0;
    glm::vec4 m_clearColor{0.f, 0.f, 0.f, 0.f};

private:
    camType m_type = camType::frustum;

    glm::mat4 m_mvp        = glm::mat4(1.f);
    glm::mat4 m_model      = glm::mat4(1.f);
    glm::mat4 m_projection = glm::mat4(1.f);
    glm::mat4 m_view       = glm::mat4(1.f);
    glm::mat3 m_normal     = glm::mat3(1.f);

    glm::vec3 m_camPos{0.f, 0.f, 1.f};
    glm::vec3 m_camLookAt{0.f};
    glm::vec3 m_camUpVec{0.f, 1.f, 0.f};
    glm::vec4 m_fishEyeParam0{0.f};
    glm::vec4 m_viewport{0.f};

    int  m_useFisheye         = 0;
    bool m_transWorldRelative = false;
    bool m_forceUpdtProjMat   = false;
    bool m_forceUpdtCb        = false;

    float m_fov            = 0.f;
    float m_aspectRatio    = 0.f;
    float m_fixAspectRatio = 0.f;
    float m_near           = 1.f;
    float m_far            = 1000.f;
    float m_left           = -0.5f;
    float m_right          = 0.5f;
    float m_bottom         = -0.5f;
    float m_top            = 0.5f;
    float m_openAngle = 1.f;
    float m_feAspect  = 1.f;

    glm::vec4 m_frustMult{};

    std::function<void(camUpt)> m_updtCb;
};
}  // namespace ara