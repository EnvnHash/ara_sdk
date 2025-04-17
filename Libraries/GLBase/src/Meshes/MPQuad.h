//
//  MPQuad.h
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//
//

#pragma once

#include "Mesh.h"

namespace ara {
class MPQuad : public Mesh {
public:
    MPQuad();
    MPQuad(float x, float y, float w, float h);
    MPQuad(float x, float y, float w, float h, glm::vec3 inNormal, float r = 1.f, float g = 1.f, float b = 1.f,
           float a = 1.f);

    virtual ~MPQuad() = default;

    void init();

private:
    float m_width  = 0.f;
    float m_height = 0.f;
};

}  // namespace ara
