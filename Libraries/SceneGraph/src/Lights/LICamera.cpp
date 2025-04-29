#include "Lights/LICamera.h"

using namespace glm;
using namespace std;

namespace ara {

LICamera::LICamera(sceneData* sd) : Light() {
    setName(getTypeName<LICamera>());
    m_transFlag |= GLSG_NO_SCALE;
    m_nodeType   = GLSG_SNT_CAMERA;
    m_aspect     = 16.f / 9.f;
    m_throwRatio = 0.5f;
    s_near       = 0.1f;
    s_far        = 1000.f;

    m_camDef = make_unique<TrackBallCam>(CameraInitParams{
        .cTyp = camType::perspective,
        .screenSize = {200.f, 200.f},
        .rect = {-m_aspect, m_aspect, -1.0f, 1.0f},    // left, right, bottom, top
    });
}

void LICamera::setup(bool callSetupCb) {
    m_trackBallChanged = false;

    // camera matrices = node matrices
    if (m_camDef) {
        bool isInvalid = std::isnan(m_rotAngle) || glm::all(glm::isnan(m_rotAxis));
        if (!isInvalid) {
            extractEulerAngleYXZ(m_rotMat, ea.y, ea.x, ea.z);

            isInvalid = glm::all(glm::isnan(ea));

            // only update is really necessary
            if (!isInvalid && !(glm::all(glm::equal(m_camDef->getTrackBallTrans(), m_transVec)) &&
                                glm::all(glm::equal(m_camDef->getTrackBallRot(), ea)))) {
                m_camDef->setTrackBallTrans(&m_transVec);  // also save to trackBall settings for
                                                           // readback on camera swap
                m_camDef->setTrackBallRot(&ea);            // also save to trackBall settings for readback on
                                                           // camera swap

                // be sure the modelMat is up-to-date
                if (m_hasNewModelMat) modelMat = m_transMat * m_rotMat * m_scaleMat;

                m_invMat = glm::inverse(modelMat);
                m_camDef->setModelMatr(m_invMat);
                m_trackBallChanged = true;
            }
        } else {
            LOG << "LICamera::setup got invalid values in m_rotAngle or "
                   "m_rotAxis)";
        }
    }

    s_fov        = m_forceFov ? m_forceFov : (float)atan(1.0 / (double)(2.0 * m_throwRatio * m_aspect));
    m_newProjMat = perspective(s_fov, m_aspect, s_near, s_far);

    // only update if necessary
    if (!glm::all(glm::equal(m_newProjMat, s_proj_mat)) || m_camDef->forceUpdtProjMat() || m_trackBallChanged ||
        callSetupCb) {
        s_proj_mat = m_newProjMat;
        m_camDef->setProjMatr(s_proj_mat);
        m_camDef->setForceUpdtProjMat(false);

        // calls m_sceneCamSet->buildCamMatrixArrays();
        if (callSetupCb || m_camDef->getForceUpdtCb()) {
            for (const auto& it : m_setupCb) {
                it();
            }
            m_camDef->setForceUpdtCb(false);
        }
    }
}

/** \brief create a shadered FBO from the pointer received here, create a
 * textureview on the specific layer, if there are more than one */
void LICamera::setCsFboPtr(FBO* fbo, int layerNr) {
    if (m_sceneCamFbo.isInited()) {
        m_sceneCamFbo.remove();
    }

    if (layerNr > -1) {
        m_sceneCamFbo.fromSharedExtractLayer(fbo, layerNr);
    } else {
        m_sceneCamFbo.fromShared(fbo);
    }
}

}  // namespace ara
