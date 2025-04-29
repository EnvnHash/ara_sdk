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
    m_circ            = make_unique<MPCircle>();
    m_format        = m_mesh->getFormat();
    m_color         = { 1.f, 1.f, 1.f, 1.f };
    m_instAttribs     = nullptr;
    m_maxNrInstances  = 1;

    Circle::init();
}

Circle::Circle(int nrSegs, float outerRad, float innerRad, float angle, glm::vec4 col,
               std::vector<CoordType> *instAttribs, int nrInstances)
    : GeoPrimitive() {
    m_circ          = make_unique<MPCircle>(nrSegs, outerRad, innerRad, angle, col.r, col.g, col.b, col.a);
    m_format        =    m_mesh->getFormat();
    m_color             = { 1.f, 1.f, 1.f, 1.f };
    m_instAttribs         = instAttribs;
    m_maxNrInstances    = nrInstances;

    Circle::init();
}

void Circle::init() {
    GLenum usage = GL_DYNAMIC_DRAW;
    if (m_instAttribs) {
        usage = GL_DYNAMIC_DRAW;
    }

    m_vao = make_unique<VAO>(m_format, usage, m_instAttribs, m_maxNrInstances);
    m_vao->setStaticColor(m_color);
    m_vao->uploadMesh(m_mesh.get());
}

}  // namespace ara
