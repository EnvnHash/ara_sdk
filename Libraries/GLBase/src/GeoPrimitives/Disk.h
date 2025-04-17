//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include "GeoPrimitives/GeoPrimitive.h"

namespace ara {

class Disk : public GeoPrimitive {
public:
    explicit Disk(float width = 2.f, float height = 2.f, int nrSubDiv = 1,
                  std::vector<CoordType> *instAttribs = nullptr, int maxNrInstances = 1, float r = 1.f, float g = 1.f,
                  float b = 1.f, float a = 1.f);

    ~Disk() = default;

    void init() override;

private:
    float                                     m_width    = 0.f;
    float                                     m_height   = 0.f;
    int                                       m_nrSubDiv = 0;
    std::vector<std::pair<glm::vec3, float> > m_sides;
    std::vector<CoordType>                   *m_instAttribs    = nullptr;
    int                                       m_maxNrInstances = 1;
};

}  // namespace ara
