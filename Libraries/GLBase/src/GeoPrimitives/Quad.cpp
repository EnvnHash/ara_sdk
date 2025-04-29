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
//  creates a quad consisting of two triangles
//  x, y means the lower left corner

#include "GeoPrimitives/Quad.h"

#include <Meshes/Mesh.h>

using namespace std;
using namespace glm;

namespace ara {

Quad::Quad() : GeoPrimitive() {
    m_size              = {1.f, 1.f};
    m_instAttribs       = nullptr;
    m_maxNrInstances    = 1;
    m_color             = { 1.f, 1.f, 1.f, 1.f };

    // first triangle, lower left, starting in the upper left corner,/ counterclockwise
    m_position.reserve(6);
    m_texCoords.reserve(6);
    m_normal.reserve(6);

    m_position.emplace_back(-0.5f, 0.5f, 0.f);
    m_texCoords.emplace_back(0.f, 1.f);

    m_position.emplace_back(-0.5f, -0.5f, 0.f);
    m_texCoords.emplace_back(0.f, 0.f);

    m_position.emplace_back(0.5f, -0.5f, 0.f);
    m_texCoords.emplace_back(1.f, 0.f);

    // second triangle, right upper, starting in the lower right corner, counterclockwise
    m_position.emplace_back(0.5f, -0.5f, 0.f);
    m_texCoords.emplace_back(0.f, 0.f);

    m_position.emplace_back(0.5f, 0.5f, 0.f);
    m_texCoords.emplace_back(1.f, 1.f);

    m_position.emplace_back(-0.5f, 0.5f, 0.f);
    m_texCoords.emplace_back(0.f, 1.f);

    // set normal facing outwards of the screen
    for (auto i = 0; i < 6; i++) {
        m_normal.emplace_back(0.f, 0.f, 1.f);
    }

    Quad::init();
}

Quad::Quad(const QuadInitParams& qd)
    : GeoPrimitive(),
        m_size(qd.size),
        m_instAttribs(qd.instAttribs),
        m_maxNrInstances(qd.nrInstances) {

    m_color = qd.color;

    glm::vec3 upperLeft{qd.pos.x, qd.pos.y + qd.size.y, 0.f};
    glm::vec3 lowerLeft{qd.pos.x, qd.pos.y, 0.f};
    glm::vec3 lowerRight{qd.pos.x + qd.size.x, qd.pos.y, 0.f};
    glm::vec3 upperRight{qd.pos.x + qd.size.x, qd.pos.y + qd.size.y, 0.f};

    // first triangle, lower left, starting in the upper left corner, counterclockwise
    m_position.reserve(6);
    m_texCoords.reserve(6);
    m_normal.reserve(6);

    m_position.emplace_back(upperLeft);
    m_texCoords.emplace_back(0.f, !qd.flipHori ? 1.f : 0.f);

    m_position.emplace_back(lowerLeft);
    m_texCoords.emplace_back(0.f, !qd.flipHori ? 0.f : 1.f);

    m_position.emplace_back(lowerRight);
    m_texCoords.emplace_back(1.f, !qd.flipHori ? 0.f : 1.f);

    // second triangle, right upper, starting in the lower right corner, counterclockwise
    m_position.emplace_back(lowerRight);
    m_texCoords.emplace_back(1.f, !qd.flipHori ? 0.f : 1.f);

    m_position.emplace_back(upperRight);
    m_texCoords.emplace_back(1.f, !qd.flipHori ? 1.f : 0.f);

    m_position.emplace_back(upperLeft);
    m_texCoords.emplace_back(0.f, !qd.flipHori ? 1.f : 0.f);

    // set normal facing outwards of the screen
    for (auto i = 0; i < 6; i++) {
        m_normal.emplace_back(qd.inNormal);
    }

    Quad::init();
}

void Quad::init() {
    Mesh m("position:3f,normal:3f,texCoord:2f,color:4f");

    // move the quad to the correct position, in relation to the dimensions
    for (unsigned short i = 0; i < 6; i++) {
        m.push_back_positions(&m_position[i][0], 3);
        m.push_back_normals(&m_normal[i][0], 3);
        m.push_back_texCoords(&m_texCoords[i][0], 2);
    }

    GLenum usage = GL_DYNAMIC_DRAW;
    if (m_instAttribs) {
        usage = GL_DYNAMIC_DRAW;
    }

    m_vao = make_unique<VAO>("position:3f,normal:3f,texCoord:2f,color:4f", usage, m_instAttribs, m_maxNrInstances);
    m_vao->setStaticColor(m_color);
    m_vao->uploadMesh(&m);
}

std::vector<glm::vec3> *Quad::getPositions() {
    return &m_position;
}

std::vector<glm::vec3> *Quad::getNormals() {
    return &m_normal;
}

std::vector<glm::vec2> *Quad::getTexCoords() {
    return &m_texCoords;
}

}  // namespace ara
