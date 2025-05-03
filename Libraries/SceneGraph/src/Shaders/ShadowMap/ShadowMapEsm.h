//
//  SHShadow.h
//
//  Created by Sven Hahne on 17.07.14.
//

#pragma once

#include "Shaders/ShadowMap/ShadowMap.h"

namespace ara {

class sceneData;
class Camera;

class ShadowMapEsm : public ShadowMap {
public:
    ShadowMapEsm(CameraSet* cs, int scrWidth, int scrHeight, glm::vec3 lightPos, float near_lim, float far_lim,
                 sceneData* scd);
    ~ShadowMapEsm() override = default;

    void                    begin() override;
    void                    end() override;
    void                    setLightPos(glm::vec3 pos);
    float                   getCoef();
    [[nodiscard]] GLenum    getColorBufType() const;

private:
    Camera* lightCam = nullptr;
    GLenum  colBufType = 0;
};
}  // namespace ara
