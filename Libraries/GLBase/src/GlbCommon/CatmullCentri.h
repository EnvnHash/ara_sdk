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
