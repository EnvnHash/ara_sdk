//
// Created by sven on 18-10-22.
//

#pragma once

#include "Utils/Stereo/LensDistortion.h"
#include "Utils/Stereo/StereoDeviceParams.h"
#include "Utils/VAO.h"

namespace ara {

class StereoRendererGeneral {
public:
    void   init(void *extData = nullptr);
    float *getEyeLensTrans(stereoEye eye);

    VAO& getVao(int i) { return m_vao[i]; }

    [[nodiscard]] std::array<float, 4> &getFov(stereoEye i) const { return m_lensDist->getFov(i); }
    [[nodiscard]] static float          getViewEyeOffs(stereoEye eye) { return LensDistortion::getViewEyeOffs(eye); }

    void setScreenParam(glm::vec2 dpi, glm::vec2 screen_size) {
        m_scrPar.m_xdpi        = dpi.x;
        m_scrPar.m_ydpi        = dpi.y;
        m_scrPar.m_screen_size = screen_size;
    }

private:
    StereoDeviceParams              m_devPar{};
    StereoScreenParams              m_scrPar{};
    std::unique_ptr<LensDistortion> m_lensDist;
    std::array<VAO, 2>              m_vao;
    std::array<glm::mat4, 2>        m_perspMat = {glm::mat4(1.f), glm::mat4(1.f)};
    std::array<glm::mat4, 2>        m_transMat = {glm::mat4(1.f), glm::mat4(1.f)};
};

}  // namespace ara
