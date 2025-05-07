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

#include "CatmullCentri.h"

using namespace glm;
using namespace std;

namespace ara {

void catmullRomCentriCalcCoeff(SplineSegment &seg, vec2 **points, float tension) {
    float alpha = 0.5f;
    float t01   = pow(distance(*points[0], *points[1]), alpha);
    float t12   = pow(distance(*points[1], *points[2]), alpha);
    float t23   = pow(distance(*points[2], *points[3]), alpha);

    // seg.c = m1
    seg.c = (1.0f - tension) *
            (*points[2] - *points[1] +
             t12 * ((*points[1] - *points[0]) / t01 - (*points[2] - *points[0]) / std::max(t01 + t12, static_cast<float>(1e-7))));
    vec2 m2 = (1.0f - tension) *
              (*points[2] - *points[1] +
               t12 * ((*points[3] - *points[2]) / t23 - (*points[3] - *points[1]) / std::max(t12 + t23, static_cast<float>(1e-7))));

    seg.a = 2.f * (*points[1] - *points[2]) + seg.c + m2;
    seg.b = -3.f * (*points[1] - *points[2]) - seg.c - seg.c - m2;
    seg.d = *points[1];
}

void catmullRomCentriCalcCoeff2(SplineSegment *seg, vector<vec2> *points) {
    float dX, dY, t0, t1, t2, t3, f;
    float alpha = 0.25f;

    t0 = 0.0f;
    dX = points->at(1).x - points->at(0).x;
    dY = points->at(1).y - points->at(0).y;
    t1 = t0 + std::max(std::pow((dX * dX + dY * dY), alpha), static_cast<float>(1e-7));
    dX = points->at(2).x - points->at(1).x;
    dY = points->at(2).y - points->at(1).y;
    t2 = t1 + std::max(std::pow((dX * dX + dY * dY), alpha), static_cast<float>(1e-7));
    dX = points->at(3).x - points->at(2).x;
    dY = points->at(3).y - points->at(2).y;
    t3 = t2 + std::max(std::pow((dX * dX + dY * dY), alpha), static_cast<float>(1e-7));

    float t01 = t0 - t1;
    float t02 = t0 - t2;
    float t03 = t0 - t3;
    float t12 = t1 - t2;
    float t13 = t1 - t3;
    float t21 = t2 - t1;
    float t23 = t2 - t3;
    float t32 = t3 - t2;

    f = t01 * t02 * t13 * t32;

    seg->d.x = points->at(1).x;
    seg->c.x =
        (t13 * t23 *
         (points->at(0).x * t12 * t12 + points->at(1).x * t02 * (t0 - 2.0f * t1 + t2) - points->at(2).x * t01 * t01)) /
        f;
    seg->b.x = (t21 * (2.0f * points->at(0).x * t12 * t13 * t23 + points->at(1).x * (t0 + t1 - 2.0f * t3) * t02 * t32 +
                       t01 * (points->at(2).x * (t0 + t2 - 2.0f * t3) * t13 + points->at(3).x * t02 * t21))) /
               f;
    seg->a.x = (t12 * (points->at(0).x * t12 * t13 * t23 + points->at(1).x * t02 * t03 * t32 +
                       t01 * (points->at(2).x * t03 * t13 + points->at(3).x * t02 * t21))) /
               f;

    seg->d.y = points->at(1).y;
    seg->c.y =
        (t13 * t23 *
         (points->at(0).y * t12 * t12 + points->at(1).y * t02 * (t0 - 2.0f * t1 + t2) - points->at(2).y * t01 * t01)) /
        f;
    seg->b.y = (t21 * (2.0f * points->at(0).y * t12 * t13 * t23 + points->at(1).y * (t0 + t1 - 2.0f * t3) * t02 * t32 +
                       t01 * (points->at(2).y * (t0 + t2 - 2.0f * t3) * t13 + points->at(3).y * t02 * t21))) /
               f;
    seg->a.y = (t12 * (points->at(0).y * t12 * t13 * t23 + points->at(1).y * t02 * t03 * t32 +
                       t01 * (points->at(2).y * t03 * t13 + points->at(3).y * t02 * t21))) /
               f;
}

void catmullRomCentriCalcCoeff2(SplineSegment &seg, vec2 **points) {
    float dX, dY, t0, t1, t2, t3, f;
    float alpha = 0.25f;

    t0 = 0.0f;
    dX = points[1]->x - points[0]->x;
    dY = points[1]->y - points[0]->y;
    t1 = t0 + std::max(std::pow((dX * dX + dY * dY), alpha), static_cast<float>(1e-7));
    dX = points[2]->x - points[1]->x;
    dY = points[2]->y - points[1]->y;
    t2 = t1 + std::max(std::pow((dX * dX + dY * dY), alpha), static_cast<float>(1e-7));
    dX = points[3]->x - points[2]->x;
    dY = points[3]->y - points[2]->y;
    t3 = t2 + std::max(std::pow((dX * dX + dY * dY), alpha), static_cast<float>(1e-7));

    float t01 = t0 - t1;
    float t02 = t0 - t2;
    float t03 = t0 - t3;
    float t12 = t1 - t2;
    float t13 = t1 - t3;
    float t21 = t2 - t1;
    float t23 = t2 - t3;
    float t32 = t3 - t2;

    f = t01 * t02 * t13 * t32;

    seg.d.x = points[1]->x;
    seg.c.x = (t13 * t23 *
               (points[0]->x * t12 * t12 + points[1]->x * t02 * (t0 - 2.0f * t1 + t2) - points[2]->x * t01 * t01)) /
              f;
    seg.b.x = (t21 * (2.0f * points[0]->x * t12 * t13 * t23 + points[1]->x * (t0 + t1 - 2.0f * t3) * t02 * t32 +
                      t01 * (points[2]->x * (t0 + t2 - 2.0f * t3) * t13 + points[3]->x * t02 * t21))) /
              f;
    seg.a.x = (t12 * (points[0]->x * t12 * t13 * t23 + points[1]->x * t02 * t03 * t32 +
                      t01 * (points[2]->x * t03 * t13 + points[3]->x * t02 * t21))) /
              f;

    seg.d.y = points[1]->y;
    seg.c.y = (t13 * t23 *
               (points[0]->y * t12 * t12 + points[1]->y * t02 * (t0 - 2.0f * t1 + t2) - points[2]->y * t01 * t01)) /
              f;
    seg.b.y = (t21 * (2.0f * points[0]->y * t12 * t13 * t23 + points[1]->y * (t0 + t1 - 2.0f * t3) * t02 * t32 +
                      t01 * (points[2]->y * (t0 + t2 - 2.0f * t3) * t13 + points[3]->y * t02 * t21))) /
              f;
    seg.a.y = (t12 * (points[0]->y * t12 * t13 * t23 + points[1]->y * t02 * t03 * t32 +
                      t01 * (points[2]->y * t03 * t13 + points[3]->y * t02 * t21))) /
              f;
}

void catmullRomCentri(std::vector<glm::vec2> &inPoints, std::vector<glm::vec2> &outPoints, unsigned int dstNrPoints,
                      float tension, int mode) {
    glm::vec2 startP, endP;
    outPoints.resize(dstNrPoints);
    float nrIntPointPerSeg = dstNrPoints / static_cast<float>(inPoints.size() - 1);

    unsigned int offs = 0;
    unsigned int sum  = 0;
    unsigned int diff = 0;

    // go through each pair of points and interpolate
    for (size_t segNr = 0; segNr < inPoints.size() - 1; segNr++) {
        SplineSegment segment;
        glm::vec2    *points[4];

        // correct the rounding errors from float to integer, add addtional
        // interation steps when we are behind the exact destination index.
        sum += (int)nrIntPointPerSeg + diff;
        diff              = static_cast<int>((segNr + 1) * nrIntPointPerSeg - sum);
        unsigned int nrIt = static_cast<int>(nrIntPointPerSeg) + diff;
        float        div  = nrIntPointPerSeg + (diff - 1.f);

        // extrapolate point at the beginning by mirroring
        if (segNr == 0) {
            startP    = (inPoints[0] - inPoints[1]) + inPoints[0];
            points[0] = &startP;
        } else
            points[0] = &inPoints[segNr - 1];

        if (segNr == inPoints.size() - 2) {
            // extrapolate point at the end by mirroring
            endP      = (inPoints[inPoints.size() - 1] - inPoints[inPoints.size() - 2]) + inPoints[inPoints.size() - 1];
            points[3] = &endP;
        } else
            points[3] = &inPoints[segNr + 2];

        for (unsigned short i = 0; i < 2; i++) points[i + 1] = &inPoints[segNr + i];

        // calculate the coefficients for onces for the whole segment
        if (mode == 0)
            catmullRomCentriCalcCoeff(segment, points, tension);
        else
            catmullRomCentriCalcCoeff2(segment, points);

        for (unsigned int i = 0; i < nrIt; i++) {
            catmullRomCentriGetPoint(segment, outPoints[offs], float(i) / div);
            offs++;
        }
    }
}

}  // namespace ara
