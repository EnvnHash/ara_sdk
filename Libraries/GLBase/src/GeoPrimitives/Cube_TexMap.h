#pragma once

#include "GeoPrimitives/GeoPrimitive.h"

namespace ara {
class Cube_TexMap : public GeoPrimitive {
public:
    explicit Cube_TexMap(float _width = 2.f, float _height = 2.f, float _depth = 2.f);

    void draw(TFO *_tfo = nullptr) override;
    void init() override;

private:
    float width  = 0;
    float height = 0;
    float depth  = 0;
};
}  // namespace ara
