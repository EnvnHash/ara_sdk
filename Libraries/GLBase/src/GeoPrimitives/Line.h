//
//  Line.h
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include "GeoPrimitives/GeoPrimitive.h"

namespace ara {

class Line : public GeoPrimitive {
public:
    Line();
    explicit Line(int nrSegments);
    Line(int nrSegments, float r, float g, float b, float a);
    Line(int nrSegments, float r, float g, float b, float a, std::vector<CoordType> *instAttribs, int nrInstance);

    void init() override;

    int m_nrSegments = 0;

private:
    std::vector<CoordType> *m_instAttribs    = nullptr;
    int                     m_maxNrInstances = 1;
};
}  // namespace ara
