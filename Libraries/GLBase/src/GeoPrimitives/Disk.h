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

class Disk : public GeoPrimitive {
public:
    explicit Disk(glm::vec2 size={2.f, 2.f}, int nrSubDiv = 1,
                  std::vector<CoordType> *instAttribs = nullptr, int maxNrInstances = 1, glm::vec4 col = {1.f, 1.f, 1.f, 1.f});

    virtual ~Disk() = default;

    void init() override;

private:
    float                                     m_width    = 0.f;
    float                                     m_height   = 0.f;
    int                                       m_nrSubDiv = 0;
    std::vector<std::pair<glm::vec3, float> > m_sides;
    std::vector<CoordType>                   *m_instAttribs    = nullptr;
    int                                       m_maxNrInstances = 1;
};

}  // namespace ara
