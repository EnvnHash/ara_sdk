//
//  SHShadow.h
//
//  Created by Sven Hahne on 17.07.14.
//

#pragma once

#include "Shaders/Shaders.h"
#include "Shaders/ShadowMap/ShadowMap.h"
#include "Utils/Camera.h"
#include "Utils/FBO.h"
#include "GlbCommon/GlbCommon.h"

namespace ara {

    class sceneData;

class ShadowMapVsm : public ShadowMap {
public:
    ShadowMapVsm(Camera* gCam, int scrWidth, int scrHeight, sceneData* scd);
    ~ShadowMapVsm() override = default;

    void begin() override;
    void end() override;
    GLuint    getDepthImg() override;
    GLuint    getColorImg() override;
    glm::mat4 getLightProjMatr() const;
    glm::mat4 lightViewMatr() const;
    void      setLightPos(glm::vec3 pos);
    void      setLookAtPoint(glm::vec3 pos);
    int       getWidth() override;
    int       getHeight() override;
    float     getCoef() const;
    FBO*      getFbo() override;

private:
    std::unique_ptr<Shaders> m_shadowShader;
    Camera*                  m_gCam = nullptr;

    glm::mat4 m_mvp{};
    glm::mat3 m_mat{};
    glm::mat4 m_sceneModelMatrix{};
    glm::mat4 m_lightViewMatrix{};
    glm::mat4 m_lightProjectionMatrix{};

    glm::vec3 m_lightPosition{};
    glm::vec3 m_lookAtPoint{};
};
}  // namespace ara
