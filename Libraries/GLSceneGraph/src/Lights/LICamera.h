#pragma once

#include <Utils/FBO.h>
#include <Utils/TrackBallCam.h>

#include "GLUtils/sceneData.h"
#include "Lights/Light.h"

namespace ara {

class LICamera : public Light {
public:
    LICamera(sceneData* sd = nullptr);
    ~LICamera() = default;

    void setup(bool callSetupCb = true);
    void          setCsFboPtr(FBO* fbo, int layerNr = -1);
    void          setCsIt(std::vector<std::pair<TrackBallCam*, void*>>::iterator it) { m_csIt = it; }
    TrackBallCam* getCamDef() {
        if (m_camDef)
            return m_camDef.get();
        else
            return nullptr;
    }
    FBO* getCamFbo() { return &m_sceneCamFbo; }
    void pushSetupCb(std::function<void()> func) { m_setupCb.emplace_back(func); }
    void clearSetupCb() { m_setupCb.clear(); }

    float m_aspect           = 0.f;
    float m_throwRatio       = 0.f;
    float m_forceFov         = 0.f;
    bool  m_trackBallChanged = false;

private:
    std::unique_ptr<TrackBallCam>                          m_camDef;
    glm::mat4                                              m_newProjMat = glm::mat4(1.f);
    glm::mat4                                              m_invMat     = glm::mat4(1.f);
    std::vector<std::pair<TrackBallCam*, void*>>::iterator m_csIt;
    glm::vec3                                              ea{0.f};
    FBO                                                    m_sceneCamFbo;
    std::list<std::function<void()>>                       m_setupCb;
};

}  // namespace ara
