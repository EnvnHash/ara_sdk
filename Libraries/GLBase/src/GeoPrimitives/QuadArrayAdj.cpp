//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  creates an Array of Quads to be used with a geometry shader and input type
//  triangles_adjacency x, y means the lower left corner, nrSegs is per side
//  standard size is 2.f * 2.f from -1|-1|0 to 1|1|0

#include "QuadArrayAdj.h"

#include "Meshes/Mesh.h"
#include "Utils/VAO.h"

using namespace std;

namespace ara {
QuadArrayAdj::QuadArrayAdj()
    : GeoPrimitive(), m_x(-1.f), m_y(-1.f), m_width(1.f), m_height(1.f), m_nrSegsX(4), m_nrSegsY(4), m_totalWidth(2.f),
      m_totalHeight(2.f), m_instAttribs(nullptr), m_maxNrInstances(1), m_usage(GL_STATIC_DRAW) {
    m_r        = 1.f;
    m_g        = 1.f;
    m_b        = 1.f;
    m_a        = 1.f;
    QuadArrayAdj::init();
}

QuadArrayAdj::QuadArrayAdj(int nrSegsX, int nrSegsY, float x, float y, float w, float h, float r, float g, float b,
                           float a, std::vector<CoordType> *instAttribs, int nrInstances, GLenum usage)
    : GeoPrimitive(), m_x(x), m_y(y), m_width(w), m_height(h), m_nrSegsX(nrSegsX), m_nrSegsY(nrSegsY),
      m_totalWidth(2.f), m_totalHeight(2.f), m_instAttribs(instAttribs), m_maxNrInstances(nrInstances), m_usage(usage) {
    m_r        = r;
    m_g        = g;
    m_b        = b;
    m_a        = a;
    QuadArrayAdj::init();
}

QuadArrayAdj::QuadArrayAdj(int nrSegsX, int nrSegsY, float x, float y, float w, float h, glm::vec3 inNormal,
                           std::vector<CoordType> *instAttribs, int nrInstances, GLenum usage)
    : GeoPrimitive(), m_x(x), m_y(y), m_width(w), m_height(h), m_nrSegsX(nrSegsX), m_nrSegsY(nrSegsY),
      m_totalWidth(2.f), m_totalHeight(2.f), m_instAttribs(instAttribs), m_maxNrInstances(nrInstances), m_usage(usage), m_qaNormal(inNormal) {
    m_r        = 0.f;
    m_g        = 0.f;
    m_b        = 0.f;
    m_a        = 1.f;
    QuadArrayAdj::init();
}

void QuadArrayAdj::init() {
    GLfloat norm[3] = {0.f, 0.f, 1.f};

    float quadWidth  = m_totalWidth / static_cast<float>(m_nrSegsX);
    float quadHeight = m_totalHeight / static_cast<float>(m_nrSegsY);

    float texWidth  = 1.f / static_cast<float>(m_nrSegsX);
    float texHeight = 1.f / static_cast<float>(m_nrSegsY);

    m_mesh = std::make_unique<Mesh>("position:3f,normal:3f,texCoord:2f,color:4f");

    // make a grid with two additional quadSize horizontally and vertically
    for (auto yInd = 0; yInd < (m_nrSegsY + 3); yInd++) {
        for (auto xInd = 0; xInd < (m_nrSegsX + 3); xInd++) {
            GLfloat v[3] = {quadWidth * (xInd - 1) + m_x, quadHeight * (yInd - 1) + m_y, 0.f};
            // printf("%d %f %f %f \n", yInd * (m_nrSegsX +3) + xInd, v[0],
            // v[1], v[2]);

            m_mesh->push_back_positions(v, 3);
            m_mesh->push_back_normals(norm, 3);

            GLfloat t[2] = {texWidth * (xInd - 1), texHeight * (yInd - 1)};
            // printf("%f %f \n", t[0], t[1]);
            m_mesh->push_back_texCoords(t, 2);
        }
    }

    // write indices
    for (auto yInd = 0; yInd < m_nrSegsY; yInd++) {
        for (auto xInd = 0; xInd < (m_nrSegsX * 2); xInd++) {
            if (xInd % 2 == 0) {
                GLushort ind[6] = {static_cast<GLushort>((m_nrSegsY + 3) * (yInd + 1) + xInd / 2 + 1),
                                   static_cast<GLushort>((m_nrSegsY + 3) * (yInd + 2) + xInd / 2),
                                   static_cast<GLushort>((m_nrSegsY + 3) * (yInd + 2) + xInd / 2 + 1),
                                   static_cast<GLushort>((m_nrSegsY + 3) * (yInd + 2) + xInd / 2 + 2),
                                   static_cast<GLushort>((m_nrSegsY + 3) * (yInd + 1) + xInd / 2 + 2),
                                   static_cast<GLushort>((m_nrSegsY + 3) * yInd + xInd / 2 + 2)};
                // printf("even y:%d: x:%d : %d %d %d %d %d %d\n", yInd, xInd,
                // ind[0], ind[1], ind[2], ind[3], ind[4], ind[5]);
                m_mesh->push_back_indices(ind, 6);
            } else {
                GLushort ind[6] = {static_cast<GLushort>((m_nrSegsY + 3) * (yInd + 2) + xInd / 2 + 1),
                                   static_cast<GLushort>((m_nrSegsY + 3) * (yInd + 3) + xInd / 2 + 1),
                                   static_cast<GLushort>((m_nrSegsY + 3) * (yInd + 2) + xInd / 2 + 2),
                                   static_cast<GLushort>((m_nrSegsY + 3) * (yInd + 1) + xInd / 2 + 3),
                                   static_cast<GLushort>((m_nrSegsY + 3) * (yInd + 1) + xInd / 2 + 2),
                                   static_cast<GLushort>((m_nrSegsY + 3) * (yInd + 1) + xInd / 2 + 1)};
                // printf("odd y:%d: x:%d : %d %d %d %d %d %d\n", yInd, xInd,
                // ind[0], ind[1], ind[2], ind[3], ind[4], ind[5]);
                m_mesh->push_back_indices(ind, 6);
            }
        }
    }

    if (m_instAttribs) m_usage = GL_DYNAMIC_DRAW;

    m_vao = std::make_unique<VAO>(m_format, m_usage, m_instAttribs, m_maxNrInstances);
    m_vao->setStaticColor(m_r, m_g, m_b, m_a);
    m_vao->uploadMesh(m_mesh.get());
}

}  // namespace ara
