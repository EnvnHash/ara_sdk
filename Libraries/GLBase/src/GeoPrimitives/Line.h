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

class Line : public GeoPrimitive {
public:
    Line();
    explicit Line(int nrSegments);
    Line(int nrSegments, const glm::vec4& col);
    Line(int nrSegments, const glm::vec4& col, std::vector<CoordType> *instAttribs, int nrInstance);

    void init() override;

    int m_nrSegments = 0;

private:
    std::vector<CoordType> *m_instAttribs    = nullptr;
    int                     m_maxNrInstances = 1;
};
}  // namespace ara
