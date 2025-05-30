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

class Quad_P : public GeoPrimitive {
public:
    class Set : public std::vector<std::unique_ptr<Quad_P>> {
    public:
        int GetIndexByObjId(int objid) const;  // -1 means NOT FOUND, array starts from 0 (zero)
    };

    Quad_P(glm::vec3 center, glm::vec2 size, float dist, glm::vec3 euler, glm::vec2 t0, glm::vec2 t1,
           bool _flipHori = true, glm::vec4 color = glm::vec4(1.f, 1.f, 1.f, 1.f));

    ~Quad_P() override = default;

    static Quad_P::Set &CreateCubeMap(Set &dest, glm::vec3 center, float cube_size,
                                      glm::vec4 color = glm::vec4(1.f, 1.f, 1.f, 1.f));

    void init() override;
    [[nodiscard]] int  getObjId() const;
    int  setObjId(int id);

private:
    glm::vec2 m_Size{};
    glm::vec4 m_Color{};

    std::vector<glm::vec4> m_Position;
    glm::vec4              m_Normal{};
    std::vector<glm::vec2> m_TexCoords;

    int objId = 0;
};

}  // namespace ara
