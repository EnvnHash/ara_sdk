//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  draw as triangle fan
//

#include "Disk.h"

#include <Meshes/Mesh.h>
#include <Utils/VAO.h>

using namespace std;
using namespace glm;

namespace ara {

Disk::Disk(float width, float height, int nrSubDiv, std::vector<CoordType> *instAttribs, int maxNrInstances, float r,
           float g, float b, float a)
    : GeoPrimitive(), m_width(width), m_height(height), m_nrSubDiv(nrSubDiv) {
    m_r = r;
    m_g = g;
    m_b = b;
    m_a = a;
    Disk::init();
}

void Disk::init() {
    m_format     = "position:3f,normal:3f,texCoord:2f,color:4f";
    GLenum usage = GL_STATIC_DRAW;
    if (m_instAttribs) usage = GL_DYNAMIC_DRAW;

    m_vao = make_unique<VAO>(m_format, usage, m_instAttribs, m_maxNrInstances);

    // positions and texcoords
    std::deque<GLfloat> positions((m_nrSubDiv + 2) * 3);
    std::deque<GLfloat> texCoords((m_nrSubDiv + 2) * 2);

    // center
    for (int i = 0; i < 3; i++) positions[i] = 0.f;
    for (int i = 0; i < 2; i++) texCoords[i] = 0.5f;

    for (int i = 0; i < (m_nrSubDiv + 1); i++) {
        double alpha               = static_cast<double>(i) / static_cast<double>(m_nrSubDiv) * M_PI * 2.0;
        positions[(i + 1) * 3]     = static_cast<float>(std::cos(alpha) * m_width * 0.5f);
        positions[(i + 1) * 3 + 1] = static_cast<float>(std::sin(alpha) * m_width * 0.5f);
        positions[(i + 1) * 3 + 2] = 0.f;

        texCoords[(i + 1) * 2]     = static_cast<float>(std::cos(alpha) * 0.5f + 0.5f);
        texCoords[(i + 1) * 2 + 1] = static_cast<float>(std::sin(alpha) * 0.5f + 0.5f);
    }

    // normals
    std::deque<GLfloat> normals((m_nrSubDiv + 2) * 3);
    for (int i = 0; i < (m_nrSubDiv + 2); i++) {
        normals[i * 3]     = 0.f;
        normals[i * 3 + 1] = 0.f;
        normals[i * 3 + 2] = 1.f;
    }

    // colors
    std::deque<GLfloat> colors((m_nrSubDiv + 2) * 4);
    for (int i = 0; i < (m_nrSubDiv + 2) * 4; i++) colors[i] = 1.f;

    m_vao->upload(CoordType::Position, &positions[0], (m_nrSubDiv + 2));
    m_vao->upload(CoordType::Normal, &normals[0], (m_nrSubDiv + 2));
    m_vao->upload(CoordType::TexCoord, &texCoords[0], (m_nrSubDiv + 2));
    m_vao->upload(CoordType::Color, &colors[0], (m_nrSubDiv + 2));
}

}  // namespace ara
