//
//  SHShadow.h
//
//  Created by Sven Hahne on 17.07.14.
//

#pragma once

#include "Shaders/ShadowMap/ShadowMap.h"

namespace ara {
class ShadowMapStd : public ShadowMap {
public:
    ShadowMapStd(CameraSet* _cs, int _scrWidth, int _scrHeight, sceneData* _scd);
    ~ShadowMapStd();

    void begin();
    void end();
};
}  // namespace ara
