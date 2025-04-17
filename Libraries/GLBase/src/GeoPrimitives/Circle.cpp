//
//  Circle.cpp
//
//  Created by Sven Hahne on 20.08.14.
//
//  creates a Circle consisting of two triangles
//  x, y means the lower left corner

#include "GeoPrimitives/Circle.h"

#include <Utils/VAO.h>

#include "Meshes/MPCircle.h"

using namespace glm;
using namespace std;

namespace ara {
Circle::Circle() {
    circ     = make_unique<MPCircle>();
    m_format = m_mesh->getFormat();

    m_r = 1.f;
    m_g = 1.f;
    m_b = 1.f;
    m_a = 1.f;

    instAttribs    = nullptr;
    maxNrInstances = 1;

    init();
}

Circle::Circle(int _nrSegs, float _outerRad, float _innerRad, float _angle, float _r, float _g, float _b, float _a,
               std::vector<CoordType> *_instAttribs, int _nrInstances)
    : GeoPrimitive() {
    circ = make_unique<MPCircle>(_nrSegs, _outerRad, _innerRad, _angle, _r, _g, _b, _a);
    // m_mesh = make_unique<Mesh>(circ->getMesh());
    m_format = m_mesh->getFormat();

    m_r = _r;
    m_g = _g;
    m_b = _b;
    m_a = _a;

    instAttribs    = _instAttribs;
    maxNrInstances = _nrInstances;

    init();
}

void Circle::init() {
    GLenum usage = GL_DYNAMIC_DRAW;
    if (instAttribs) usage = GL_DYNAMIC_DRAW;

    m_vao = make_unique<VAO>(m_format, usage, instAttribs, maxNrInstances);
    m_vao->setStaticColor(m_r, m_g, m_b, m_a);
    m_vao->uploadMesh(m_mesh.get());
}

}  // namespace ara
