//
//  QuadArray.h
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//
//

#pragma once

#include "GeoPrimitives/GeoPrimitive.h"

namespace ara {

class QuadArray : public GeoPrimitive {
public:
    QuadArray(int nrSegsX, int nrSegsY, float x = -1.f, float y = -1.f, float w = 2.f, float h = 2.f, float r = 1.f,
              float g = 1.f, float b = 1.f, float a = 1.f, std::vector<CoordType> *instAttribs = nullptr,
              int nrInstances = 1, GLenum usage = GL_STATIC_DRAW);

    QuadArray(int nrSegsX, int nrSegsY, float x, float y, float w, float h, glm::vec3 inNormal,
              std::vector<CoordType> *instAttribs = nullptr, int nrInstances = 1, GLenum usage = GL_STATIC_DRAW);

    virtual ~QuadArray() = default;

    void init();

private:
    int   m_nrSegsX     = 0;
    int   m_nrSegsY     = 0;
    float m_x           = 0.f;
    float m_y           = 0.f;
    float m_totalWidth  = 2.f;
    float m_totalHeight = 2.f;

    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec3> m_normals;
    std::vector<glm::vec2> m_texCoords;
    glm::vec3              m_qaNormal{0.f, 0.f, 1.f};

    std::vector<CoordType> *m_instAttribs;
    int                     m_maxNrInstances = 1;

    GLenum m_usage = GL_STATIC_DRAW;
};
}  // namespace ara
