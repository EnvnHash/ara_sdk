#pragma once

#include "GeoPrimitives/Cube.h"
#include "Light.h"

namespace ara {

class LIStandardSpot : public Light {
public:
    LIStandardSpot(sceneData* sd = nullptr);
    ~LIStandardSpot() {};

    void setup(bool force = false);

private:
    glm::mat4 scale_bias_matrix;
    float     linearDepthScalar;
};

}  // namespace ara
