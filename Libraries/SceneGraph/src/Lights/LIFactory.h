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
    LIFactory() {}
    ~LIFactory() {}

    std::unique_ptr<Light> Create(const std::string& sClassName, sceneData* sd) {
        if (sClassName == getName<LIProjector>()) {
            return std::make_unique<LIProjector>(sd);
        } else if (sClassName == getName<LIStandardSpot>()) {
            return std::make_unique<LIStandardSpot>(sd);
        } else if (sClassName == getName<LICamera>()) {
            return std::make_unique<LICamera>(sd);
        } else {
            LOGE << "LIFactory error, Prototype: " << sClassName.c_str() << " not found";
        }
        return 0;
    };
};

}  // namespace ara
