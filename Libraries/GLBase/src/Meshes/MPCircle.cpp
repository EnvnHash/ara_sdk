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
    m_angle       = (float)TWO_PI;
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

    for (auto i = 0; i < m_nrSegQuads + 1; i++) {
        float fInd = static_cast<float>(i) / static_cast<float>(m_nrSegQuads);

        outerRing.push_back(
            vec3(std::cos(fInd * m_angle) * m_outerRadius, std::sin(fInd * m_angle) * m_outerRadius, 0.f));

        innerRing.push_back(
            vec3(std::cos(fInd * m_angle) * m_innerRadius, std::sin(fInd * m_angle) * m_innerRadius, 0.f));
    }

    // gehe die skelett punkte durch, konstruiere jeweils ein Quad
    // bestehend aus zwei Triangles pro Segment
    vec3 nrml;
    for (int i = 0; i < m_nrSegQuads; i++) {
        float fInd        = static_cast<float>(i) / static_cast<float>(m_nrSegQuads);
        float fIndPlusOne = static_cast<float>(i + 1) / static_cast<float>(m_nrSegQuads);

        for (uint j = 0; j < 3; j++) m_positions.push_back(outerRing[i][j]);
        m_texCoords.push_back(fInd);
        m_texCoords.push_back(1.f);
        nrml = vec3(0.f, 0.f, 1.f) * (1.f - m_smoothNorm) + (glm::normalize(outerRing[i]) * m_smoothNorm);
        for (uint j = 0; j < 3; j++) m_normals.push_back(nrml[j]);

        for (uint j = 0; j < 3; j++) m_positions.push_back(innerRing[(i + 1)][j]);
        m_texCoords.push_back(fIndPlusOne);
        m_texCoords.push_back(0.f);
        nrml = vec3(0.f, 0.f, 1.f);
        for (uint j = 0; j < 3; j++) m_normals.push_back(nrml[j]);

        for (uint j = 0; j < 3; j++) m_positions.push_back(innerRing[i][j]);
        m_texCoords.push_back(fInd);
        m_texCoords.push_back(0.f);
        for (uint j = 0; j < 3; j++) m_normals.push_back(nrml[j]);

        //-----

        for (uint j = 0; j < 3; j++) m_positions.push_back(innerRing[(i + 1)][j]);
        m_texCoords.push_back(fIndPlusOne);
        m_texCoords.push_back(0.f);
        for (uint j = 0; j < 3; j++) m_normals.push_back(nrml[j]);

        for (uint j = 0; j < 3; j++) m_positions.push_back(outerRing[(i + 1)][j]);
        m_texCoords.push_back(fInd);
        m_texCoords.push_back(0.f);
        nrml = vec3(0.f, 0.f, 1.f) * (1.f - m_smoothNorm) + (glm::normalize(outerRing[i]) * m_smoothNorm);
        for (uint j = 0; j < 3; j++) m_normals.push_back(nrml[j]);

        for (uint j = 0; j < 3; j++) m_positions.push_back(outerRing[(i + 1)][j]);
        m_texCoords.push_back(fIndPlusOne);
        m_texCoords.push_back(0.f);
        for (uint j = 0; j < 3; j++) m_normals.push_back(nrml[j]);
    }

    for (auto i = 0; i < m_nrSegQuads * 6; i++) {
        // GLfloat v[3] = { positions[i*3], positions[i*3+1], positions[i*3+2]
        // };
        push_back_positions(&m_positions[i * 3], 3);

        // GLfloat n[3] = { normal[i].x, normal[i].y, normal[i].z };
        push_back_normals(&m_normals[i * 3], 3);

        // GLfloat t[2] = { texCoords[i].x, texCoords[i].y };
        push_back_texCoords(&m_texCoords[i * 2], 2);
    }
}

}  // namespace ara
