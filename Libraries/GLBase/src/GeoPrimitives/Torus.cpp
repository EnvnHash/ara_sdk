//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  creates an Array of Quads which will result in one Plane
//  x, y means the lower left corner, nrSegs is per side
//  standard size is 2.f * 2.f from -1|-1|0 to 1|1|0

#include "Torus.h"

#include <Meshes/MPQuad.h>
#include <Utils/VAO.h>

using namespace std;
using namespace glm;

namespace ara {
Torus::Torus() : GeoPrimitive() {
    m_x       = -1.f;
    m_y       = -1.f;
    m_width   = 1.f;
    m_height  = 1.f;
    m_nrSegsX = 40;
    m_nrSegsY = 4;

    m_instAttribs    = nullptr;
    m_maxNrInstances = 1;

    m_r        = 1.f;
    m_g        = 1.f;
    m_b        = 1.f;
    m_a        = 1.f;

    Torus::init();
}

Torus::Torus(int nrSegsX, int nrSegsY, float rad, std::vector<CoordType> *instAttribs, int nrInstances)
    : GeoPrimitive(), m_nrSegsX(nrSegsX), m_nrSegsY(nrSegsY), m_instAttribs(instAttribs), m_maxNrInstances(nrInstances) {
    Torus::init();
}

void Torus::init() {
    m_quads.resize(m_nrSegsX * m_nrSegsY);
    float quadWidth  = m_totalWidth / static_cast<float>(m_nrSegsX);
    float quadHeight = m_totalHeight / static_cast<float>(m_nrSegsY);
    float xOffs      = m_totalWidth / static_cast<float>(m_nrSegsX);
    float yOffs      = m_totalHeight / static_cast<float>(m_nrSegsY);

    for (auto yInd = 0; yInd < m_nrSegsY; yInd++) {
        float yo = static_cast<float>(yInd) * yOffs - (m_totalWidth * 0.5f);
        for (auto xInd = 0; xInd < m_nrSegsX; xInd++) {
            float xo                         = static_cast<float>(xInd) * xOffs - (m_totalHeight * 0.5f);
            m_quads[yInd * m_nrSegsX + xInd] = std::make_unique<MPQuad>(xo, yo, quadWidth, quadHeight, m_qaNormal);
        }
    }

    m_mesh = make_unique<Mesh>("position:3f,normal:3f,texCoord:2f,color:4f");

    // move the Torus to the correct position, in relation to the dimensions
    for (auto yInd = 0; yInd < m_nrSegsY; yInd++) {
        for (auto xInd = 0; xInd < m_nrSegsX; xInd++) {
            auto pos = m_quads[yInd * m_nrSegsX + xInd]->getPositions();
            auto nor = m_quads[yInd * m_nrSegsX + xInd]->getNormals();

            for (auto i = 0; i < 6; i++) {
                m_mesh->push_back_positions(&pos->at(i * 3), 3);
                m_mesh->push_back_normals(&nor->at(i * 3), 3);

                GLfloat t[2] = {pos->at(i * 3) / m_totalWidth + 0.5f, pos->at(i * 3 + 1) / m_totalHeight + 0.5f};
                m_mesh->push_back_texCoords(t, 2);
            }
        }
    }

    auto usage = GL_DYNAMIC_DRAW;
    if (m_instAttribs) {
        usage = GL_DYNAMIC_DRAW;
    }

    m_vao = make_unique<VAO>(m_format, usage, m_instAttribs, m_maxNrInstances);
    m_vao->setStaticColor(m_r, m_g, m_b, m_a);
    m_vao->uploadMesh(m_mesh.get());
}

}  // namespace ara
