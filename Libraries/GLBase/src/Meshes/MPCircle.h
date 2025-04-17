#pragma once

#include "Mesh.h"

namespace ara {

class MPCircle : public Mesh {
public:
    MPCircle();
    MPCircle(int nrSegs, float outerRad, float innerRad, float angle = TWO_PI, float r = 1.f, float g = 1.f,
             float b = 1.f, float a = 1.f);
    virtual ~MPCircle() = default;
    void init();
    int  getNrSegments() { return m_nrSegQuads; }

private:
    bool  m_closeCircle = false;
    int   m_nrSegQuads  = 0;
    float m_outerRadius = 0.f;
    float m_innerRadius = 0.f;
    float m_angle       = 0.f;
    float m_smoothNorm  = 0.f;

    std::vector<glm::vec3> outerRing;
    std::vector<glm::vec3> innerRing;
};
}  // namespace ara
