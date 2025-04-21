//
//  SHShadow.h
//
//  Created by Sven Hahne on 17.07.14.
//

#pragma once

#include "Shaders/ShadowMap/ShadowMap.h"

using namespace glm;

namespace ara {
class ShadowMapEsm : public ShadowMap {
public:
    ShadowMapEsm(CameraSet* _cs, int _scrWidth, int _scrHeight, vec3 _lightPos, float _near, float _far,
                 sceneData* _scd);
    ~ShadowMapEsm();

    void begin();
    void end();
    void setLightPos(vec3 _pos);
    float  getCoef();
    GLenum getColorBufType();

private:
    Camera* lightCam;
    GLenum  colBufType;
};
}  // namespace ara
