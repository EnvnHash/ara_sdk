#pragma once

#include "Light.h"

namespace ara {

class LIStandardSpot : public Light {
public:
    explicit LIStandardSpot(sceneData* sd = nullptr);

    void setup(bool force) override;

private:
    glm::mat4 scale_bias_matrix{};
    float     linearDepthScalar{};
};

}  // namespace ara
