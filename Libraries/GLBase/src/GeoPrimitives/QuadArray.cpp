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
//  creates an Array of Quads which will result in one Plane
//  x, y means the lower left corner, nrSegs is per side
//  standard size is 2.f * 2.f from -1|-1|0 to 1|1|0

#include "GeoPrimitives/QuadArray.h"

#include "Meshes/Mesh.h"
#include "Utils/VAO.h"

using namespace std;
using namespace glm;

namespace ara {
QuadArray::QuadArray(int nrSegsX, int nrSegsY, float x, float y, float w, float h, float r, float g, float b, float a,
                     std::vector<CoordType> *instAttribs, int nrInstances, GLenum usage)
    : GeoPrimitive(), m_nrSegsX(nrSegsX), m_nrSegsY(nrSegsY), m_x(x), m_y(y), m_instAttribs(instAttribs),
      m_maxNrInstances(nrInstances), m_usage(usage) {
    m_r = r;
    m_g = g;
    m_b = b;
    m_a = a;

    QuadArray::init();
}

QuadArray::QuadArray(int nrSegsX, int nrSegsY, float x, float y, float w, float h, glm::vec3 inNormal,
                     std::vector<CoordType> *instAttribs, int nrInstances, GLenum usage)
    : GeoPrimitive(), m_nrSegsX(nrSegsX), m_nrSegsY(nrSegsY), m_x(x), m_y(y), m_instAttribs(instAttribs),
      m_maxNrInstances(nrInstances), m_usage(usage) {
    m_qaNormal = inNormal;
    QuadArray::init();
}

void QuadArray::init() {
    GLfloat quadPos[18] = {0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f};
    GLfloat norm[3]     = {0.f, 0.f, 1.f};

    float quadWidth  = m_totalWidth / static_cast<float>(m_nrSegsX);
    float quadHeight = m_totalHeight / static_cast<float>(m_nrSegsY);

    float texWidth  = 1.f / static_cast<float>(m_nrSegsX);
    float texHeight = 1.f / static_cast<float>(m_nrSegsY);

    m_mesh = make_unique<Mesh>("position:3f,normal:3f,texCoord:2f,color:4f");

    // move the QuadArray to the correct position, in relation to the dimensions
    for (auto yInd = 0; yInd < m_nrSegsY; yInd++) {
        for (auto xInd = 0; xInd < m_nrSegsX; xInd++) {
            for (auto i = 0; i < 6; i++) {
                GLfloat v[3] = {quadPos[i * 3] * quadWidth + quadWidth * xInd + m_x,
                                quadPos[i * 3 + 1] * quadHeight + quadHeight * yInd + m_y, quadPos[i * 3 + 2]};
                m_mesh->push_back_positions(v, 3);
                m_mesh->push_back_normals(norm, 3);

                GLfloat t[2] = {quadPos[i * 3] * texWidth + texWidth * xInd,
                                quadPos[i * 3 + 1] * texHeight + texHeight * yInd};
                m_mesh->push_back_texCoords(t, 2);
            }
        }
    }

    if (m_instAttribs) m_usage = GL_DYNAMIC_DRAW;

    m_vao = make_unique<VAO>(m_format, m_usage, m_instAttribs, m_maxNrInstances);
    m_vao->setStaticColor(m_r, m_g, m_b, m_a);
    m_vao->uploadMesh(m_mesh.get());
}

}  // namespace ara
