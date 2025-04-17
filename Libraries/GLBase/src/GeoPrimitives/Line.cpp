//  Created by Sven Hahne on 21.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "GeoPrimitives/Line.h"

#include <Meshes/Mesh.h>
#include <Utils/VAO.h>

using namespace std;
using namespace glm;

namespace ara {

Line::Line() : GeoPrimitive(), m_nrSegments(100) {
    m_r = 1.0f;
    m_g = 1.0f;
    m_b = 1.0f;
    m_a = 1.0f;
    init();
}

Line::Line(int _nrSegments) : GeoPrimitive(), m_nrSegments(_nrSegments) {
    m_r = 1.0f;
    m_g = 1.0f;
    m_b = 1.0f;
    m_a = 1.0f;
    Line::init();
}

Line::Line(int nrSegments, float r, float g, float b, float a) : GeoPrimitive(), m_nrSegments(nrSegments) {
    m_r = r;
    m_g = g;
    m_b = b;
    m_a = a;
    Line::init();
}

Line::Line(int nrSegments, float r, float g, float b, float a, std::vector<CoordType> *instAttribs, int nrInstance)
    : GeoPrimitive(), m_nrSegments(nrSegments), m_instAttribs(instAttribs), m_maxNrInstances(nrInstance) {
    m_r = r;
    m_g = g;
    m_b = b;
    m_a = a;
    Line::init();
}

void Line::init() {
    m_format = "position:3f,color:4f";
    m_mesh   = make_unique<Mesh>(m_format);

    for (int i = 0; i < m_nrSegments; i++) {
        float fInd = static_cast<float>(i) / static_cast<float>(m_nrSegments - 1);

        // move the quad to the correct position, in relation to the dimensions
        GLfloat v[3] = {fInd * 2.f - 1.0f, 0.f, 0.f};
        m_mesh->push_back_positions(v, 3);
    }

    GLenum usage = GL_DYNAMIC_DRAW;
    if (m_instAttribs) usage = GL_DYNAMIC_DRAW;

    m_vao = make_unique<VAO>(m_format, usage, m_instAttribs, m_maxNrInstances);
    m_vao->setStaticColor(m_r, m_g, m_b, m_a);
    m_vao->uploadMesh(m_mesh.get());
}

}  // namespace ara
