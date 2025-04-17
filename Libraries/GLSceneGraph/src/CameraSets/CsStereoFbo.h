//
//  CsStereoFbo.h
//
//  Created by Sven Hahne last modified on 22.08.17
//

#pragma once

#include <Utils/FBO.h>
#include <Utils/Stereo/StereoRenderer.h>

#include "CameraSets/CameraSet.h"

#if defined(__ANDROID__) && defined(ARA_USE_ARCORE)
#include <ARCore/ARCore.h>
#include "Android/ARCoreCam.h"
#endif

namespace ara {

enum class CsStereoViewMode : int { Single = 0, Stereo };

class CsStereoFbo : public CameraSet {
public:
    CsStereoFbo(sceneData* sc);
    virtual ~CsStereoFbo() = default;

    void initLayerTexShdr();
    void initBackTexShdr(size_t nrLayers);
    void initClearShader(size_t nrLayers);
    void initPlaneShdr(size_t nrLayers);
    void initFocSquareShdr(size_t nrLayers);
    void rebuildFbo();
    void clearScreen(renderPass pass) override;
    void clearDepth() override;
    void clearFbo() override {}

    void renderTree(SceneNode* scene, double time, double dt, uint ctxNr, renderPass pass) override;
    void render(SceneNode* scene, SceneNode* parent, double time, double dt, uint ctxNr, renderPass pass) override;
    void renderBackground();
    void postRender(renderPass pass, float* extDrawMatr = nullptr /*, float* extViewport=nullptr*/) override;
    void renderFbos(float* extDrawMatr = nullptr /*, float* extViewport=nullptr*/) override;
    void setViewport(uint x, uint y, uint width, uint height, bool resizeProto = false) override;

    void setViewMode(CsStereoViewMode mode);
    void buildCamMatrixArrays() override;
    void uptStereoRender();

    FBO*     getFbo() override { return &m_fbo; }
    Shaders* getLayerTexShdr() { return m_layerTexShdr; }

#if defined(__ANDROID__) && defined(ARA_USE_ARCORE)
    void      buildViewMatrixArrays();
    void      setArCore(cap::ARCore* arCore);
    void      drawPlane(ArPlane* plane);
    void      updateForPlane(ArSession* ar_session, ArPlane* ar_plane);
    glm::vec3 getPlaneNormal(ArSession* ar_session, ArPose* plane_pose);
    void      lockOrientation();

    void showFocusCircle(bool val) { m_renderFocusSq = val; }
    void showPlanes(bool val) { m_renderPlanes = val; }
#endif

    void onKey(int key, int scancode, int action, int mods) override {}

private:
    std::array<GLint, 4>  m_csVp;
    FBO                   m_fbo;
    std::unique_ptr<Quad> m_quad;
    Shaders*              m_layerTexShdr = nullptr;
    Shaders*              m_clearShdr    = nullptr;
    Shaders*              m_backTexShdr  = nullptr;
    Shaders*              m_focSqShdr    = nullptr;
    Shaders*              m_colShdr      = nullptr;
    glm::vec3             m_camPos;
    StereoRenderer        m_stereoRenderer;
    Texture*              m_backTex          = nullptr;
    float                 m_stereoViewAspect = 1.f;

    std::unique_ptr<VAO>     m_plane;
    std::unique_ptr<Texture> m_triTex;
    Shaders*                 m_planeShdr = nullptr;
    CsStereoViewMode         m_viewMode  = CsStereoViewMode::Stereo;

#if defined(__ANDROID__) && defined(ARA_USE_ARCORE)
    cap::ARCore* m_arCore = nullptr;
    ARCoreCam    m_arCoreCam;

    ArTrackable*    m_ar_trackable  = nullptr;
    ArPlane*        m_ar_plane      = nullptr;
    ArPlane*        m_subsume_plane = nullptr;
    ArTrackingState m_out_tracking_state;
    ArAnchor*       m_refAnchor = nullptr;

    bool m_renderFocusSq     = true;
    bool m_renderPlanes      = true;
    bool m_arCamPersSet      = false;
    bool refPlaneSet         = false;
    bool m_uvs_initialized   = false;
    bool m_orientationLocked = false;

    int32_t m_geometry_changed = 0;

    glm::mat4 m_focus_circle_mat = glm::mat4(1.f);
    glm::mat4 m_foc_circ_pvm[2]  = {glm::mat4(1.f), glm::mat4(1.f)};
    glm::mat4 m_mvp[2]           = {glm::mat4(1.f), glm::mat4(1.f)};
    glm::mat4 m_model_mat        = glm::mat4(1.f);
    glm::vec3 m_normal_vec{0.f};

    std::vector<glm::vec3> m_vertices;
    std::vector<GLuint>    m_indices;

    std::array<GLfloat, 12> m_Vertices = {-1.f, 1.f, -1.f, -1.f, 1.f, -1.f, 1.f, -1.f, 1.f, 1.f, -1.f, 1.f};
    std::array<GLfloat, 12> m_transformed_uvs;

    const float kFeatherLength = 0.2f;  // Feather distance 0.2 meters.
    const float kFeatherScale  = 0.2f;  // Feather scale over the distance
                                        // between plane center and vertices.
#endif
};

}  // namespace ara
