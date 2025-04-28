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


#pragma once

#include "GeoPrimitives/GeoPrimitive.h"

namespace ara {

struct QuadArrayInitParams {
    int nrSegsX=0;
    int nrSegsY=0;
    float x = -1.f;
    float y = -1.f;
    float w = 2.f;
    float h = 2.f;
    std::vector<CoordType> *instAttribs = nullptr;
    int nrInstances = 1;
    GLenum usage = GL_STATIC_DRAW;
    glm::vec4 color =  {1.f, 1.f, 1.f, 1.f };
};

class QuadArray : public GeoPrimitive {
public:
    QuadArray(const QuadArrayInitParams& ip);
    virtual ~QuadArray() = default;

    void init();

private:
    int   m_nrSegsX     = 0;
    int   m_nrSegsY     = 0;
    float m_x           = 0.f;
    float m_y           = 0.f;
    float m_totalWidth  = 2.f;
    float m_totalHeight = 2.f;

    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec3> m_normals;
    std::vector<glm::vec2> m_texCoords;
    glm::vec3              m_qaNormal{0.f, 0.f, 1.f};

    std::vector<CoordType> *m_instAttribs;
    int                     m_maxNrInstances = 1;

    GLenum m_usage = GL_STATIC_DRAW;
};
}  // namespace ara
