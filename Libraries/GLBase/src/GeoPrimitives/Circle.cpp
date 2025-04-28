//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
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
    circ            = make_unique<MPCircle>();
    m_format        = m_mesh->getFormat();
    m_color         = { 1.f, 1.f, 1.f, 1.f };
    instAttribs     = nullptr;
    maxNrInstances  = 1;

    Circle::init();
}

Circle::Circle(int _nrSegs, float _outerRad, float _innerRad, float _angle, glm::vec4 col,
               std::vector<CoordType> *_instAttribs, int _nrInstances)
    : GeoPrimitive() {
    circ = make_unique<MPCircle>(_nrSegs, _outerRad, _innerRad, _angle, col.r, col.g, col.b, col.a);
    m_format        = m_mesh->getFormat();
    m_color         = { 1.f, 1.f, 1.f, 1.f };
    instAttribs     = _instAttribs;
    maxNrInstances  = _nrInstances;

    Circle::init();
}

void Circle::init() {
    GLenum usage = GL_DYNAMIC_DRAW;
    if (instAttribs) {
        usage = GL_DYNAMIC_DRAW;
    }

    m_vao = make_unique<VAO>(m_format, usage, instAttribs, maxNrInstances);
    m_vao->setStaticColor(m_color);
    m_vao->uploadMesh(m_mesh.get());
}

}  // namespace ara
