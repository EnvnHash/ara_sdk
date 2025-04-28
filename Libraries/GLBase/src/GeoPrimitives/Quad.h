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
#include "Utils/VAO.h"

namespace ara {

struct QuadInitParams {
    glm::vec2 pos = {-1.f, -1.f };
    glm::vec2 size= { 2.f, 2.f };
    glm::vec3 inNormal{0.f, 0.f, 1.f};
    glm::vec4 color { 1.f, 1.f, 1.f, 1.f };
    std::vector<CoordType>* instAttribs = nullptr;
    int nrInstances = 1;
    bool flipHori = false;
};

class Quad : public GeoPrimitive {
public:
    Quad();
    Quad(const QuadInitParams&);
    virtual ~Quad() = default;

    void init();

    void drawAsShared() {
        if (!m_vao) return;
        m_vao->enableVertexAttribs();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_vao->disableVertexAttribs();
    }

    std::vector<glm::vec3> *getPositions();
    std::vector<glm::vec3> *getNormals();
    std::vector<glm::vec2> *getTexCoords();

private:
    glm::vec2              m_size{};
    std::vector<glm::vec3> m_position;
    std::vector<glm::vec3> m_normal;
    std::vector<glm::vec2> m_texCoords;

    std::vector<CoordType> *m_instAttribs;
    int                     m_maxNrInstances=0;
};
}  // namespace ara
