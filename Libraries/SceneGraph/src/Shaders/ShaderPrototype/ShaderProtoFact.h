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
    static std::unique_ptr<ShaderProto> Create(const std::string& sClassName, sceneData* sd) {
        static const std::unordered_map<std::string, std::function<std::unique_ptr<ShaderProto>(sceneData*)>> creatorMap = {
            {getTypeName<SPNoLight>(), [](sceneData* sd){ return std::make_unique<SPNoLight>(sd); }},
            {getTypeName<SPDirLight>(), [](sceneData* sd){ return std::make_unique<SPDirLight>(sd); }},
            {getTypeName<SPSpotLight>(), [](sceneData* sd){ return std::make_unique<SPSpotLight>(sd); }},
            {getTypeName<SPSpotLightShadow>(), [](sceneData* sd){ return std::make_unique<SPSpotLightShadow>(sd); }},
            {getTypeName<SPSpotLightShadowVsm>(), [](sceneData* sd){ return std::make_unique<SPSpotLightShadowVsm>(sd); }},
            {getTypeName<SPGridFloorAxes>(), [](sceneData* sd){ return std::make_unique<SPGridFloorAxes>(sd); }},
            {getTypeName<SPGridFloor>(), [](sceneData* sd){ return std::make_unique<SPGridFloor>(sd); }},
            {getTypeName<SPWorldAxes>(), [](sceneData* sd){ return std::make_unique<SPWorldAxes>(sd); }},
            {getTypeName<SPObjectSelector>(), [](sceneData* sd){ return std::make_unique<SPObjectSelector>(sd); }},
            {getTypeName<SPSkyBox>(), [](sceneData* sd){ return std::make_unique<SPSkyBox>(sd); }}
        };

        auto it = creatorMap.find(sClassName);
        if (it != creatorMap.end()) {
            return it->second(sd);
        } else {
            LOGE << "ShaderProtoFact error, Prototype: " << sClassName.c_str() << " not found";
            return nullptr;
        }
    };
};

}  // namespace ara
