//
// Created by sven on 7/2/22.
//

#pragma once

#ifdef ARA_USE_ARCORE
#include <ARCore/ARCore.h>

#include "Image.h"

namespace ara {

class UIWindow;

class ARCoreCam : public Image {
public:
    ARCoreCam();
    ARCoreCam(std::string&& styleClass);
    virtual ~ARCoreCam() = default;

    void createArSession();
    void init();
    void initGLRes(int nrLayers);

    Shaders* getCamShdr(int nrLayers);
    Shaders* getPlaneShdr();
    Shaders* getFocSquareShdr();

    bool draw(uint32_t* objId);
    bool drawIndirect(uint32_t* objId);
    bool drawFunc(uint32_t* objId);
    void      drawPlane(ArPlane* plane);
    void      updateForPlane(ArSession* ar_session, ArPlane* ar_plane);
    glm::vec3 getPlaneNormal(ArSession* ar_session, ArPose* plane_pose);

    void updateAR();
    bool createBgQuad();

    void showFocusCircle(bool val) { m_renderFocusSq = val; }
    void setDataPath(std::string str) { m_dataPath = str; }
    void setWin(UIWindow* win) { m_win = win; }

    cap::ARCore* m_arCore = nullptr;

private:
    uint32_t                 m_tempObjId = 0;
    uint32_t                 m_dfObjId   = 0;
    Shaders*                 m_camShdr;
    Shaders*                 m_planeShdr;
    Shaders*                 m_focSqShdr;
    std::unique_ptr<VAO>     m_bgQuad;
    std::unique_ptr<VAO>     m_plane;
    std::unique_ptr<Quad>    m_quad;
    std::unique_ptr<Texture> m_triTex;
    glm::vec3                m_normal_vec;
    glm::mat4                m_model_mat;
    glm::mat4                m_focus_circle_mat;
    glm::mat4                m_foc_circ_pvm;
    std::vector<glm::vec3>   m_vertices;
    std::vector<GLuint>      m_indices;

#ifndef ARA_ANDROID_PURE_NATIVE_APP
    JNIEnv* m_jniEnv = nullptr;
#else
    ANativeActivity* m_activity = nullptr;
#endif

    std::array<GLfloat, 8> m_Vertices = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};
    std::array<GLfloat, 8> m_transformed_uvs;
    bool                   m_uvs_initialized = false;
    bool                   m_renderPlanes    = true;
    bool                   m_renderFocusSq   = true;

    ArTrackable*    m_ar_trackable  = nullptr;
    ArPlane*        m_ar_plane      = nullptr;
    ArPlane*        m_subsume_plane = nullptr;
    ArTrackingState m_out_tracking_state;

    const float kFeatherLength = 0.2f;  // Feather distance 0.2 meters.
    const float kFeatherScale  = 0.2f;  // Feather scale over the distance
                                        // between plane center and vertices.
    glm::mat4   m_mvp;
    std::string m_dataPath;
    UIWindow*   m_win = nullptr;
};

}  // namespace ara
#endif
