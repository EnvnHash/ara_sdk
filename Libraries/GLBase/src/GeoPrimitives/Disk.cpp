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

#include "Disk.h"

#include <Meshes/Mesh.h>
#include <Utils/VAO.h>

using namespace std;
using namespace glm;

namespace ara {

Disk::Disk(vec2 size, int nrSubDiv, std::vector<CoordType> *instAttribs, int maxNrInstances, vec4 col)
    : GeoPrimitive(), m_width(size.x), m_height(size.y), m_nrSubDiv(nrSubDiv) {
    m_color = col;
    Disk::init();
}

void Disk::init() {
    m_format     = "position:3f,normal:3f,texCoord:2f,color:4f";
    auto usage = m_instAttribs ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    m_vao = make_unique<VAO>(m_format, usage, m_instAttribs, m_maxNrInstances);

    // positions and texcoords
    std::deque<GLfloat> positions((m_nrSubDiv + 2) * 3);
    std::deque<GLfloat> texCoords((m_nrSubDiv + 2) * 2);

    // center
    for (int i = 0; i < 3; i++) {
        positions[i] = 0.f;
    }

    for (int i = 0; i < 2; i++) {
        texCoords[i] = 0.5f;
    }

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
