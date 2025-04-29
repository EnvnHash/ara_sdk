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

#include "GeoPrimitives/Line.h"

#include <Meshes/Mesh.h>
#include <Utils/VAO.h>

using namespace std;
using namespace glm;

namespace ara {

Line::Line() : GeoPrimitive(), m_nrSegments(100) {
    m_color = { 1.f, 1.f, 1.f, 1.f };
    Line::init();
}

Line::Line(int _nrSegments) : GeoPrimitive(), m_nrSegments(_nrSegments) {
    m_color = { 1.f, 1.f, 1.f, 1.f };
    Line::init();
}

Line::Line(int nrSegments, const vec4& col) : GeoPrimitive(), m_nrSegments(nrSegments) {
    m_color = col;
    Line::init();
}

Line::Line(int nrSegments, const vec4& col, std::vector<CoordType> *instAttribs, int nrInstance)
    : GeoPrimitive(), m_nrSegments(nrSegments), m_instAttribs(instAttribs), m_maxNrInstances(nrInstance) {
    m_color = col;
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

    m_vao = make_unique<VAO>(m_format, GL_DYNAMIC_DRAW, m_instAttribs, m_maxNrInstances);
    m_vao->setStaticColor(m_color);
    m_vao->uploadMesh(m_mesh.get());
}

}  // namespace ara
