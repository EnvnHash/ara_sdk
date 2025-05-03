//
//  SHShadow.h
//
//  Created by Sven Hahne on 17.07.14.
//

#pragma once

#include "Shaders/ShadowMap/ShadowMap.h"

namespace ara {

    class sceneData;

class ShadowMapStd : public ShadowMap {
public:
    ShadowMapStd(CameraSet* cs, int scrWidth, int scrHeight, sceneData* scd);
    ~ShadowMapStd() override = default;

    void begin() override;
    void end() override;
};
}  // namespace ara
