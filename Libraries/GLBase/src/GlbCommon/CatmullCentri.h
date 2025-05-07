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

#include "GlbCommon/GlbCommon.h"

namespace ara {

struct SplineSegment {
    glm::vec2 a;
    glm::vec2 b;
    glm::vec2 c;
    glm::vec2 d;
};

void catmullRomCentriCalcCoeff(SplineSegment &seg, glm::vec2 **points, float tension);
void catmullRomCentriCalcCoeff2(SplineSegment &seg, glm::vec2 **points);
void catmullRomCentriCalcCoeff2(SplineSegment *seg, std::vector<glm::vec2> *points);
void catmullRomCentri(std::vector<glm::vec2> &inPoints, std::vector<glm::vec2> &outPoints, unsigned int dstNrPoints,
                      float tension, int mode);

inline void catmullRomCentriGetPoint(SplineSegment &seg, glm::vec2 &point, float t) {
    point = seg.a * t * t * t + seg.b * t * t + seg.c * t + seg.d;
}

inline void catmullRomCentriGetPoint(SplineSegment *seg, glm::vec2 &point, float t) {
    point = seg->a * t * t * t + seg->b * t * t + seg->c * t + seg->d;
}

}  // namespace ara
