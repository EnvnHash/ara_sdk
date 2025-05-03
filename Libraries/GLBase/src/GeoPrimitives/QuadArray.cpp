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
QuadArray::QuadArray(const QuadArrayInitParams& ip)
    : GeoPrimitive(),
        m_nrSegsX(ip.nrSegsX),
        m_nrSegsY(ip.nrSegsY),
        m_x(ip.x),
        m_y(ip.y),
        m_instAttribs(ip.instAttribs),
        m_maxNrInstances(ip.nrInstances),
        m_usage(ip.usage) {
    m_color = ip.color;
    QuadArray::init();
}


void QuadArray::init() {
    std::array<vec3, 6> quadPos = {
        vec3{0.f, 1.f, 0.f},
        vec3{0.f, 0.f, 0.f},
        vec3{1.f, 0.f, 0.f},
        vec3{1.f, 0.f, 0.f},
        vec3{1.f, 1.f, 0.f},
        vec3{0.f, 1.f, 0.f} };
    vec3 norm = {0.f, 0.f, 1.f};

    vec2 quadSize { m_totalWidth / static_cast<float>(m_nrSegsX),
                    m_totalHeight / static_cast<float>(m_nrSegsY) };

    vec2 texSize { 1.f / static_cast<float>(m_nrSegsX),
                   1.f / static_cast<float>(m_nrSegsY) };

    m_mesh = make_unique<Mesh>("position:3f,normal:3f,texCoord:2f,color:4f");

    // move the QuadArray to the correct position, in relation to the dimensions
    for (auto yInd = 0; yInd < m_nrSegsY; ++yInd) {
        for (auto xInd = 0; xInd < m_nrSegsX; ++xInd) {
            for (auto i = 0; i < 6; i++) {
                vec3 v = {  quadPos[i].x * quadSize.x + quadSize.x * xInd + m_x,
                            quadPos[i].y * quadSize.y + quadSize.y * yInd + m_y,
                            quadPos[i].z};
                m_mesh->push_back_positions(&v[0], 1);
                m_mesh->push_back_normals(&norm[0], 1);

                vec2 t = {quadPos[i].x * texSize.x + texSize.x * xInd,
                          quadPos[i].y * texSize.y + texSize.y * yInd};
                m_mesh->push_back_texCoords(&t[0], 1);
            }
        }
    }

    if (m_instAttribs) {
        m_usage = GL_DYNAMIC_DRAW;
    }

    m_vao = make_unique<VAO>(m_format, m_usage, m_instAttribs, m_maxNrInstances);
    m_vao->setStaticColor(m_color);
    m_vao->uploadMesh(m_mesh.get());
}

}  // namespace ara
