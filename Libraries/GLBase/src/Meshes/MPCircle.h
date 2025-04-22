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

#include "Meshes/Mesh.h"

namespace ara {

class MPCircle : public Mesh {
public:
    MPCircle();
    MPCircle(int nrSegs, float outerRad, float innerRad, float angle = TWO_PI, float r = 1.f, float g = 1.f,
             float b = 1.f, float a = 1.f);
    ~MPCircle() override = default;
    void init();
    void addCirclePoints(float radius, int i, float fInd);
    void calculateNormals(int i, float fIndPlusOne, float fInd);
    void push_back_positions(const float* pos, int count);
    void push_back_normals(const float* normal, int count);
    void push_back_texCoords(const float* texCoord, int count);

    [[nodiscard]] int  getNrSegments() const { return m_nrSegQuads; }

private:
    bool  m_closeCircle = false;
    int   m_nrSegQuads  = 0;
    float m_outerRadius = 0.f;
    float m_innerRadius = 0.f;
    float m_angle       = 0.f;
    float m_smoothNorm  = 0.f;

    std::vector<glm::vec3> outerRing;
    std::vector<glm::vec3> innerRing;
};
}  // namespace ara
