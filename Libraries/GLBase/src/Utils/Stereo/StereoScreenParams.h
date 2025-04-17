//
// Created by sven on 18-10-22.
//

#pragma once

#include <glb_common/glb_common.h>

namespace ara {

class StereoScreenParams {
public:
    glm::vec2 getScreenSizeInMeters() const {
        return glm::vec2{(m_screen_size.x / m_xdpi) * kMetersPerInch, (m_screen_size.y / m_ydpi) * kMetersPerInch};
    }

    float               m_xdpi = 0.f;
    float               m_ydpi = 0.f;
    glm::vec2           m_screen_size{0.f};
    static inline float kMetersPerInch = 0.0254f;
};

}  // namespace ara