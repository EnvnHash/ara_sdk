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

class Quad : public GeoPrimitive {
public:
    Quad();

    Quad(float x, float y, float w, float h, glm::vec3 inNormal = glm::vec3(0.f, 0.f, 1.f), float _r = 1.f,
         float _g = 1.f, float _b = 1.f, float _a = 1.f, std::vector<CoordType> *_instAttribs = nullptr,
         int _nrInstances = 1, bool _flipHori = false);

    virtual ~Quad() {}

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
    float                  width;
    float                  height;
    std::vector<glm::vec3> position;
    std::vector<glm::vec3> normal;
    std::vector<glm::vec2> texCoords;

    std::vector<CoordType> *instAttribs;
    int                     maxNrInstances;
};
}  // namespace ara
