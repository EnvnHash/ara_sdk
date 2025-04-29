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
class MPCircle;
class Circle : public GeoPrimitive {
public:
    Circle();
    Circle(int nrSegs, float outerRad, float innerRad, float angle = TWO_PI, glm::vec4 col = {1.f, 1.f, 1.f, 1.f},
            std::vector<CoordType> *instAttribs = nullptr, int nrInstances = 1);

    virtual ~Circle() = default;

    void init() override;

private:
    std::unique_ptr<MPCircle> m_circ;
    std::vector<CoordType>   *m_instAttribs;
    int                       m_maxNrInstances = 1;
};
}  // namespace ara
