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

#include "GeoPrimitives/Cylinder.h"

#include <glsg_common/glsg_common.h>

#include "Utils/VAO.h"

using namespace std;
using namespace glm;

namespace ara {

Cylinder::Cylinder(unsigned int nrSegs, std::vector<CoordType> *instAttribs, int maxNrInstances)
    : m_instAttribs(instAttribs), m_maxNrInstances(maxNrInstances), m_nrSegs(nrSegs) {
    Cylinder::init();
}

void Cylinder::init() {
    float cylRadius = 1.f;

    // the cylinder will be normalized from (-1|-1|-1) to (1|1|1)
    // triangles with indices define Triangles
    // we will define the cylinder body and the caps separately so that we can use smooth normals (and have a hard edge
    // on the top -> so different normals at same positions)

    // define a ring in the x, z plain
    const auto ringPos = get2DRing(static_cast<int>(m_nrSegs) * 2);

    // ------- cylinder body
    // allocate memory for all positions and normals
    // two rings with each three coordinates (x, y, z)
    // two caps with each three coordinates (x, y, z)
    // one center point on each cap

    unsigned int totNrVert = m_nrSegs * 4 + 2;
    auto        positions = std::vector<vec3>(totNrVert);
    auto        normals   = std::vector<vec3>(totNrVert);

    auto posIt = positions.begin();
    auto normIt = normals.begin();

    for (unsigned int ringNr = 0; ringNr < 2; ringNr++) {
        buildRing(m_nrSegs, posIt, normIt, ringPos, cylRadius, ringNr == 0 ? -1.f : 1.f);
    }

    createCapCenters(posIt, normIt);

    for (unsigned int ringNr = 0; ringNr < 2; ringNr++) {
        buildRing(m_nrSegs, posIt, normIt, ringPos, cylRadius,  ringNr == 0 ? -1.f : 1.f
                  /*, vec3{0.f, ringNr == 0 ? -1.f : 1.f, 0.f} */);
    }

    // create Indices
    std::vector<GLuint> cyl_indices(m_nrSegs * 12);

    //  clockwise (viewed from the camera)
    std::array<GLuint, 6> oneQuadTemp = {0, 0, 1, 1, 0, 1};
    std::array<GLuint, 6> upDownTemp  = {0, 1, 0, 0, 1, 1};  // 0 = bottom, 1 ==top

    uint ind = 0;
    for (auto i = 0; i < m_nrSegs; i++) {
        for (auto j = 0; j < 6; j++) {
            cyl_indices[ind++] = ((oneQuadTemp[j] + i) % m_nrSegs) + (m_nrSegs * upDownTemp[j]);
        }
    }

    // cap bottom and cap top
    unsigned int capCenterInd = m_nrSegs * 2;

    // cap bottom and top = 2
    for (unsigned int k = 0; k < 2; k++) {
        auto posIndOffs = m_nrSegs * 2 + 2 + m_nrSegs * k;

        for (auto i = 0; i < m_nrSegs; i++) {
            for (auto j = 0; j < 3; j++) {
                switch (j) {
                    case 0:
                        cyl_indices[ind++] = capCenterInd + k;
                        break;  // always center of cap / tip
                    case 1:
                        cyl_indices[ind++] = ((k == 1 ? i + 1 : i) % m_nrSegs) + posIndOffs;
                        break;
                    case 2:
                        cyl_indices[ind++] = ((k == 1 ? i : i + 1) % m_nrSegs) + posIndOffs;
                        break;
                }
            }
        }
    }

    GLenum usage = GL_STATIC_DRAW;
    if (m_instAttribs) {
        usage = GL_DYNAMIC_DRAW;
    }

    m_vao = make_unique<VAO>("position:4f,normal:2f", usage, m_instAttribs, m_maxNrInstances, true);
    m_vao->upload(CoordType::Position, &positions[0][0], totNrVert);
    m_vao->upload(CoordType::Normal, &normals[0][0], totNrVert);
    m_vao->setElemIndices(m_nrSegs * 12, &cyl_indices[0]);

    m_totNrPoints = totNrVert;
}

void Cylinder::createCapCenters(std::vector<vec3>::iterator& pos, std::vector<vec3>::iterator& norm) {
    for (unsigned int i = 0; i < 2; i++) {
        *pos = {0.f, i == 0 ? -1.f : 1.f, 0.f};
        *norm = {0.f, i == 0 ? -1.f : 1.f, 0.f};
    }
}

}  // namespace ara
