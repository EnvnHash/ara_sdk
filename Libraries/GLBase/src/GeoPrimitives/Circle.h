//
//  Circle.h
//
//  Created by Sven Hahne on 20.08.14.
//

#pragma once

#include "GeoPrimitives/GeoPrimitive.h"

namespace ara {
class MPCircle;
class Circle : public GeoPrimitive {
public:
    Circle();

    Circle(int _nrSegs, float _outerRad, float _innerRad, float _angle = TWO_PI, float _r = 1.f, float _g = 1.f,
           float _b = 1.f, float _a = 1.f, std::vector<CoordType> *_instAttribs = nullptr, int _nrInstances = 1);

    virtual ~Circle() = default;

    void init() override;

private:
    std::unique_ptr<MPCircle> circ;
    std::vector<CoordType>   *instAttribs;
    int                       maxNrInstances = 1;
};
}  // namespace ara
