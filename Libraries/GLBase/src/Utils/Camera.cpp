#include "Utils/Camera.h"

#include <Log.h>

using namespace glm;
using namespace std;

namespace ara {

Camera::Camera(camType cTyp, float screenWidth, float screenHeight, float left, float right, float bottom, float top,
               float cpX, float cpY, float cpZ, float laX, float laY, float laZ, float upX, float upY, float upZ,
               float inNear, float inFar, float fov)
    : m_type(cTyp), m_screenWidth(screenWidth), m_screenHeight(screenHeight), m_left(left), m_right(right),
      m_bottom(bottom), m_top(top), m_near(inNear), m_far(inFar), m_camPos(vec3(cpX, cpY, cpZ)),
      m_camLookAt(vec3(laX, laY, laZ)), m_camUpVec(vec3(upX, upY, upZ)), m_fov(glm::radians(fov)) {
    buildMatrices();
}

void Camera::init(bool fromChangedScreenSize) {
    switch (m_type) {
        case camType::perspective: break;
        case camType::frustum: break;
        case camType::ortho:
            m_near   = 0.0f;
            m_far    = 1000.0f;
            m_left   = -1.0f;
            m_right  = 1.0f;
            m_bottom = -1.0f;
            m_top    = 1.0f;
            break;
        default: break;
    }

    buildMatrices(fromChangedScreenSize);
}

void Camera::buildMatrices(bool fromChangedScreenSize) {
    switch (m_type) {
        case camType::perspective:
            // m_fov in radians
            if (m_screenHeight != 0) m_aspectRatio = m_screenWidth / m_screenHeight;
            if (m_fixAspectRatio != 0.f) m_aspectRatio = m_fixAspectRatio;
            m_projection = perspective(m_fov, m_aspectRatio, m_near, m_far);
            break;
        case camType::frustum:
            m_projection = frustum(m_left + m_frustMult[3], m_right + m_frustMult[1], m_bottom + m_frustMult[0],
                                   m_top + m_frustMult[2], m_near, m_far);
            break;
        case camType::ortho:
            m_projection = ortho(m_left, m_right, m_bottom, m_top, m_near,
                                 m_far);  // In world coordinates
            break;
        default: break;
    }

    // in case init() was called from fromChangedScreenSize, only matrices with
    // values related to the screensize should be rebuild
    if (!fromChangedScreenSize) {
        m_model  = mat4(1.f);
        m_view   = lookAt(m_camPos, m_camLookAt, m_camUpVec);
        m_normal = mat3(transpose(inverse(m_model)));
    }

    // Our ModelViewProjection : multiplication of our 3 matrices
    m_mvp = m_projection * m_view * m_model;
}

void Camera::setupPerspective(float _fov, float nearDist, float farDist, int _scrWidth, int _scrHeight) {
    m_fov  = _fov;
    m_near = nearDist;
    m_far  = farDist;

    float eyeX = static_cast<float>(_scrWidth) * 0.5f;
    float eyeY = static_cast<float>(_scrHeight) * 0.5f;
    // float halfFov = PI * m_fov / 360;

    auto  tan_fovy = (float)tan(radians(m_fov * 0.5));
    float dist     = eyeY / tan_fovy;

    if (m_near == 0) m_near = dist / 10.0f;
    if (m_far == 0) m_far = dist * 10.0f;

    m_aspectRatio =
        m_fixAspectRatio != 0.f ? m_fixAspectRatio : static_cast<float>(_scrWidth) / static_cast<float>(_scrHeight);

    m_right  = tan_fovy * m_aspectRatio * m_near;
    m_left   = -m_right;
    m_top    = tan_fovy * m_near;
    m_bottom = -m_top;

    m_camPos    = vec3(eyeX, eyeY, dist);
    m_camLookAt = vec3(eyeX, eyeY, 0);
}

void Camera::setScreenSize(uint width, uint height) {
    m_screenWidth  = (float)width;
    m_screenHeight = (float)height;
    init(true);
}

void Camera::modelTrans(float x, float y, float z) {
    m_model = translate(m_model, vec3(x, y, z));
    m_mvp   = m_projection * m_view * m_model;  // Remember, matrix multiplication is the other way around
    if (m_updtCb) m_updtCb(camUpt::ModelTrans);
}

void Camera::modelRot(float angle, float x, float y, float z) {
    m_model = rotate(m_model, angle, vec3(x, y, z));
    m_mvp   = m_projection * m_view * m_model;
    if (m_updtCb) m_updtCb(camUpt::ModelRot);
}

void Camera::setModelMatr(mat4 &modelMatr) {
    // only update if it is really a new model mat
    if (!glm::all(glm::equal(modelMatr, m_model))) {
        m_model  = modelMatr;
        m_normal = mat3(transpose(inverse(m_model)));
        m_mvp    = m_projection * m_view * m_model;
        if (m_updtCb) m_updtCb(camUpt::ModelMat);
    }
}

void Camera::setViewMatr(mat4 &viewMatr) {
    m_view = viewMatr;
    m_mvp  = m_projection * m_view * m_model;
    if (m_updtCb) m_updtCb(camUpt::ViewMat);
}

void Camera::setProjMatr(mat4 &projMatr) {
    m_projection = projMatr;
    m_mvp        = m_projection * m_view * m_model;
    if (m_updtCb) m_updtCb(camUpt::ProjMat);
}

void Camera::setFrustMult(const float *_multVal) {
    for (int i = 0; i < 4; i++) m_frustMult[i] = _multVal[i];
    buildMatrices();
    if (m_updtCb) m_updtCb(camUpt::FrustMult);
}

// cam
bool Camera::setFishEyeParam() {
    double c_MinNumber = 1.e-7;
    double c_PIH       = M_PI / 2.0;
    double camW        = m_screenWidth;

    if (!((camW > c_MinNumber) && (m_feAspect > c_MinNumber) && (m_feAspect > c_MinNumber))) {
        m_fishEyeParam0[0] = 1.0f;
        m_fishEyeParam0[1] = 1.0f;
        m_fishEyeParam0[2] = (float)c_PIH;
        m_fishEyeParam0[3] = 0.0f;

        return false;
    }

    double r;
    auto   camH = (double)glm::round(camW / m_feAspect);

    if (m_feAspect >= 1.0) {
        camH /= 2.0;
        r                  = (camH - (double)m_borderPix) / camH;
        m_fishEyeParam0[0] = (float)(r / m_feAspect);
        m_fishEyeParam0[1] = (float)r;
    } else {
        camW /= 2.0;
        r                  = (camW - (double)m_borderPix) / camW;
        m_fishEyeParam0[0] = (float)r;
        m_fishEyeParam0[1] = (float)(r * m_feAspect);
    }

    m_fishEyeParam0[2] = (float)(m_openAngle / 2.0);
    m_fishEyeParam0[3] = 0.0f;

    return true;
}

void Camera::debug() {
    LOG << "model Matrix: " << to_string(m_model).c_str();
    LOG << "view Matrix: " << to_string(m_view).c_str();
    LOG << "projection Matrix: " << to_string(m_projection).c_str();
    LOG << "s_camPos: " << to_string(m_camPos).c_str();
    LOG << "s_lookAt: " << to_string(m_camLookAt).c_str();
    LOG << "near: " << m_near << " far: " << m_far;
}

}  // namespace ara
