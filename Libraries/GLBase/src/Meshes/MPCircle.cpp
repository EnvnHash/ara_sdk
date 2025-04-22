//
//  creates a MPCircle consisting of two triangles
//  x, y means the lower left corner

#include "Meshes/MPCircle.h"

using namespace glm;
using namespace std;

namespace ara {
MPCircle::MPCircle() : Mesh() {
    m_outerRadius = 0.5f;
    m_innerRadius = 0.1f;
    m_smoothNorm  = 0.5f;

    m_nrSegQuads  = 20;
    m_angle       = static_cast<float>(TWO_PI);
    m_closeCircle = true;

    m_r = 1.f;
    m_g = 1.f;
    m_b = 1.f;
    m_a = 1.f;

    MPCircle::init();
}

MPCircle::MPCircle(int nrSegs, float outerRad, float innerRad, float angle, float r, float g, float b, float a)
    : Mesh() {
    m_outerRadius = outerRad;
    m_innerRadius = innerRad;
    m_nrSegQuads  = nrSegs;
    m_angle       = angle;
    m_r           = r;
    m_g           = g;
    m_b           = b;
    m_a           = a;

    m_smoothNorm  = 0.5f;
    m_closeCircle = m_angle >= TWO_PI ? true : false;

    MPCircle::init();
}

void MPCircle::init() {
    m_format = "position:3f,normal:3f,texCoord:2f,color:4f";

    for (int i = 0; i <= m_nrSegQuads; ++i) {
        float fInd = static_cast<float>(i) / static_cast<float>(m_nrSegQuads);
        addCirclePoints(m_outerRadius, i, fInd);
        addCirclePoints(m_innerRadius, i, fInd);
    }

    for (int i = 0; i < m_nrSegQuads; ++i) {
        float fIndPlusOne = static_cast<float>(i + 1) / static_cast<float>(m_nrSegQuads);
        calculateNormals(i, fIndPlusOne, static_cast<float>(i) / static_cast<float>(m_nrSegQuads));
    }

    for (int i = 0; i < m_nrSegQuads * 6; ++i) {
        push_back_positions(&m_positions[i * 3], 3);
        push_back_normals(&m_normals[i * 3], 3);
        push_back_texCoords(&m_texCoords[i * 2], 2);
    }
}

void MPCircle::addCirclePoints(float radius, int i, float fInd) {
    outerRing.emplace_back(
        std::cos(fInd * m_angle) * radius,
        std::sin(fInd * m_angle) * radius,
        0.f
    );
}

void MPCircle::calculateNormals(const int i, float fIndPlusOne, float fInd) {
    // Calculate normals for the current segment
    auto normal = vec3{0.f, 0.f, 1.f} * (1.f - m_smoothNorm) + (glm::normalize(outerRing[i]) * m_smoothNorm);
    push_back_normals(&normal[0], 3);

    // Repeat the same normal for other vertices in this segment
    push_back_normals(&normal[0], 3);
    push_back_normals(&normal[0], 3);

    normal = vec3{0.f, 0.f, 1.f};
    push_back_normals(&normal[0], 3);
    push_back_normals(&normal[0], 3);
    push_back_normals(&normal[0], 3);
}

void MPCircle::push_back_positions(const float* pos, int count) {
    for (int i = 0; i < count; ++i) {
        m_positions.push_back(pos[i]);
    }
}

void MPCircle::push_back_normals(const float* normal, int count) {
    for (int i = 0; i < count; ++i) {
        m_normals.push_back(normal[i]);
    }
}

void MPCircle::push_back_texCoords(const float* texCoord, int count) {
    for (int i = 0; i < count; ++i) {
        m_texCoords.push_back(texCoord[i]);
    }
}

}  // namespace ara
