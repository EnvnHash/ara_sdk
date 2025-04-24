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

#pragma once

#include "GeoPrimitives/GeoPrimitive.h"

namespace ara {

class QuadArrayAdj : public GeoPrimitive {
public:
    QuadArrayAdj();

    QuadArrayAdj(glm::ivec2 nrSegs, glm::vec2 pos={-1.f, -1.f}, glm::vec2 size={2.f, 2.f}, glm::vec4={1.f,1.f, 1.f, 1.f},
                 std::vector<CoordType> *instAttribs = nullptr, int nrInstances = 1, GLenum usage = GL_STATIC_DRAW);

    QuadArrayAdj(glm::ivec2 nrSegs, glm::vec2 pos={-1.f, -1.f}, glm::vec2 size={2.f, 2.f}, glm::vec3 inNormal={0.f, 0.f, 1.f},
                 std::vector<CoordType> *instAttribs = nullptr, int nrInstances = 1, GLenum usage = GL_STATIC_DRAW);

    virtual ~QuadArrayAdj() = default;

    void init();

private:
    int m_nrSegsX = 0;
    int m_nrSegsY = 0;

    float m_x           = 0.f;
    float m_y           = 0.f;
    float m_width       = 0.f;
    float m_height      = 0.f;
    float m_totalWidth  = 0.f;
    float m_totalHeight = 0.f;

    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec3> m_normals;
    std::vector<glm::vec2> m_texCoords;
    glm::vec3              m_qaNormal{0.f, 0.f, 1.f};

    std::vector<CoordType> *m_instAttribs    = nullptr;
    int                     m_maxNrInstances = 1;

    GLenum m_usage;
};
}  // namespace ara
