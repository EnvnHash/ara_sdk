#pragma once

#include "GeoPrimitives/GeoPrimitive.h"

namespace ara {
class Cube : public GeoPrimitive {
public:
    explicit Cube(float width = 2.f, float height = 2.f, float depth = 2.f);

    void draw(TFO *_tfo = nullptr) override;
    void init() override {}
};

}  // namespace ara
