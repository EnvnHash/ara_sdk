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
//
//  creates a MPQuad consisting of two triangles
//  x, y means the lower left corner

#include "Meshes/MPQuad.h"

using namespace glm;
using namespace std;

namespace ara {

MPQuad::MPQuad() : MPQuad(0.f, 0.f, 1.f, 1.f) {}

MPQuad::MPQuad(float x, float y, float w, float h) : MPQuad(x, y, w, h, glm::vec3(0.f, 0.f, 1.f), glm::vec4(1.f)) {}

MPQuad::MPQuad(float x, float y, float w, float h, glm::vec3 inNormal, const glm::vec4& col) {
    m_width = w;
    m_height = h;
    m_r = col.r;
    m_g = col.g;
    m_b = col.b;
    m_a = col.a;

    // get the rotation matrix for the projection from inNormal to the standard normal
    auto rot = RotationBetweenVectors(vec3{0.f, 0.f, 1.f}, inNormal);
    auto rMatr = inNormal != vec3{0.f, 0.f, -1.f} ? glm::mat4_cast(rot) :
                 glm::rotate(mat4(1.f), static_cast<float>(M_PI), vec3{0.f, 1.f, 0.f});

    setPositions(x, y, w, h, rMatr);
    setNormals();

    init();
}

void MPQuad::setPositions(float x, float y, float w, float h, const glm::mat4& rMatr) {
    vec3 upperLeft = vec3(x + w / 2, y + h / 2, 0.f);
    vec3 lowerLeft = vec3(x - w / 2, y - h / 2, 0.f);
    vec3 lowerRight = vec3(x + w / 2, y - h / 2, 0.f);
    vec3 upperRight = vec3(x - w / 2, y + h / 2, 0.f);

    auto applyRotation = [](const glm::mat4& rMatr, const vec3& point) {
        glm::vec4 tempV = (rMatr * vec4(point, 1.f));
        return vec3(tempV.x, tempV.y, tempV.z);
    };

    upperLeft = applyRotation(rMatr, upperLeft);
    lowerLeft = applyRotation(rMatr, lowerLeft);
    lowerRight = applyRotation(rMatr, lowerRight);
    upperRight = applyRotation(rMatr, upperRight);

    // first triangle
    m_positions.insert(m_positions.end(), &upperLeft[0], &upperLeft[3]);
    m_texCoords.push_back(0.f);
    m_texCoords.push_back(1.f);

    m_positions.insert(m_positions.end(), &lowerLeft[0], &lowerLeft[3]);
    m_texCoords.push_back(0.f);
    m_texCoords.push_back(0.f);

    m_positions.insert(m_positions.end(), &lowerRight[0], &lowerRight[3]);
    m_texCoords.push_back(1.f);
    m_texCoords.push_back(0.f);

    // second triangle
    m_positions.insert(m_positions.end(), &lowerRight[0], &lowerRight[3]);
    m_texCoords.push_back(1.f);
    m_texCoords.push_back(0.f);

    m_positions.insert(m_positions.end(), &upperRight[0], &upperRight[3]);
    m_texCoords.push_back(1.f);
    m_texCoords.push_back(1.f);

    m_positions.insert(m_positions.end(), &upperLeft[0], &upperLeft[3]);
    m_texCoords.push_back(0.f);
    m_texCoords.push_back(1.f);
}

void MPQuad::setNormals() {
    for (auto i = 0; i < 6; i++) {
        m_normals.push_back(0.f);
        m_normals.push_back(0.f);
        m_normals.push_back(1.f);
    }
}

void MPQuad::init() {
    m_format = "position:3f,normal:3f,texCoord:2f,color:4f";

    // move the MPQuad to the correct position, in relation to the dimensions
    for (auto i = 0; i < 6; i++) {
        push_back_positions(&m_positions[i * 3], 3);
        push_back_normals(&m_normals[i * 3], 3);
        push_back_texCoords(&m_texCoords[i * 2], 2);
    }
}

}  // namespace ara
