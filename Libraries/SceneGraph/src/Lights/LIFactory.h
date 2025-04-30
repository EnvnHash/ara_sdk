//
//  Author Sven Hahne
//

#pragma once

#include "Lights/LICamera.h"
#include "Lights/LIProjector.h"
#include "Lights/LIStandardSpot.h"

namespace ara {

class LIFactory {
public:
    using LightFactory = std::function<std::unique_ptr<Light>(sceneData*)>;

    std::unique_ptr<Light> Create(const std::string& sClassName, sceneData* sd) {
        static const std::unordered_map<std::string, LightFactory> lightFactories = {
            {getTypeName<LIProjector>(), [](sceneData* s) -> std::unique_ptr<Light> { return std::make_unique<LIProjector>(s); }},
            {getTypeName<LIStandardSpot>(), [](sceneData* s) -> std::unique_ptr<Light> { return std::make_unique<LIStandardSpot>(s); }},
            {getTypeName<LICamera>(), [](sceneData* s) -> std::unique_ptr<Light> { return std::make_unique<LICamera>(s); }}
        };

        auto it = lightFactories.find(sClassName);
        if (it != lightFactories.end()) {
            return it->second(sd);
        } else {
            LOGE << "LIFactory error, Prototype: " << sClassName.c_str() << " not found";
            return nullptr;
        }
    }
};

}  // namespace ara
