//
//  Author: Sven Hahne
//

#pragma once

#include "Shaders/ShaderPrototype/SPDirLight.h"
#include "Shaders/ShaderPrototype/SPGridFloor.h"
#include "Shaders/ShaderPrototype/SPGridFloorAxes.h"
#include "Shaders/ShaderPrototype/SPNoLight.h"
#include "Shaders/ShaderPrototype/SPObjectSelector.h"
#include "Shaders/ShaderPrototype/SPSkyBox.h"
#include "Shaders/ShaderPrototype/SPSpotLight.h"
#include "Shaders/ShaderPrototype/SPSpotLightShadow.h"
#include "Shaders/ShaderPrototype/SPSpotLightShadowVsm.h"
#include "Shaders/ShaderPrototype/SPWorldAxes.h"
#include "Shaders/ShaderPrototype/ShaderProto.h"

namespace ara {
class ShaderProtoFact {
public:
    ShaderProtoFact() {}
    virtual ~ShaderProtoFact() {}

    std::unique_ptr<ShaderProto> Create(const std::string& sClassName, sceneData* sd) {
        if (sClassName == getTypeName<SPNoLight>()) {
            return std::make_unique<SPNoLight>(sd);
        } else if (sClassName == getTypeName<SPDirLight>()) {
            return std::make_unique<SPDirLight>(sd);
        } else if (sClassName == getTypeName<SPSpotLight>()) {
            return std::make_unique<SPSpotLight>(sd);
        } else if (sClassName == getTypeName<SPSpotLightShadow>()) {
            return std::make_unique<SPSpotLightShadow>(sd);
        } else if (sClassName == getTypeName<SPSpotLightShadowVsm>()) {
            return std::make_unique<SPSpotLightShadowVsm>(sd);
        } else if (sClassName == getTypeName<SPGridFloorAxes>()) {
            return std::make_unique<SPGridFloorAxes>(sd);
        } else if (sClassName == getTypeName<SPGridFloor>()) {
            return std::make_unique<SPGridFloor>(sd);
        } else if (sClassName == getTypeName<SPWorldAxes>()) {
            return std::make_unique<SPWorldAxes>(sd);
        } else if (sClassName == getTypeName<SPObjectSelector>()) {
            return std::make_unique<SPObjectSelector>(sd);
        } else if (sClassName == getTypeName<SPSkyBox>()) {
            return std::make_unique<SPSkyBox>(sd);
        } else {
            LOGE << "ShaderProtoFact error, Prototype: " << sClassName.c_str() << " not found";
        }
        return 0;
    };
};
}  // namespace ara
