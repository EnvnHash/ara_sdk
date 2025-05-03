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

namespace ara {

class sceneData;

class ShadowMapEsm2 : public ShadowMap {
public:
    ShadowMapEsm2(Camera* gCam, int scrWidth, int scrHeight, glm::vec3 lightPos, float near_lim, float far_lim,
                  sceneData* scd);
    ~ShadowMapEsm2() override = default;

    void begin() override;
    void end() override;

    GLuint                  getDepthImg() override;
    GLuint                  getColorImg() override;
    [[nodiscard]] glm::mat4 getLightProjMatr() const;
    [[nodiscard]] glm::mat4 lightViewMatr() const;
    float*                  getShadowMatr();
    void                    setLightPos(glm::vec3 pos);
    void                    setLookAtPoint(glm::vec3 pos);
    int                     getWidth() override;
    int                     getHeight() override;
    [[nodiscard]] float     getCoef() const;
    FBO*                    getFbo() override;
    [[nodiscard]] float     getLinearDepthScalar() const;
    [[nodiscard]] GLenum    getColorBufType() const;

private:
    Shaders*                m_depthShader = nullptr;
    Camera*                 m_gCam = nullptr;
    Camera*                 m_lightCam = nullptr;
    std::unique_ptr<FBO>    m_fbo;
    glm::mat4               m_mvp{};
    glm::mat3               m_mat{};
    glm::mat4               m_lightViewMatrix{};
    glm::mat4               m_lightProjectionMatrix{};
    glm::mat4               m_shadowMatr{};
    glm::mat4               m_scaleBiasMatrix{};

    glm::vec3 m_lightPosition{};
    glm::vec3 lookAtPoint{};

    int nmatLoc = 0;

    float linearDepthScalar = 0;
    float f_near = 0;
    float f_far = 0;

    GLenum colBufType{};
};
}  // namespace ara
