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

#include "GeoPrimitives/Quad_P.h"

#include <Meshes/Mesh.h>
#include <Utils/VAO.h>

using namespace glm;
using namespace std;

namespace ara {

Quad_P::Quad_P(glm::vec3 center, glm::vec2 size, float dist, glm::vec3 euler, glm::vec2 t0, glm::vec2 t1, bool _flipHori, glm::vec4 color)
    : GeoPrimitive() {
    m_Size = size;

    auto imat = glm::eulerAngleXYZ(euler.x, euler.y, euler.z);
    m_Normal  = imat * vec4(0, 0, 1, 1);
    imat      = glm::translate(imat, vec3(0, 0, 0 + dist));

    m_Color = color;

    vec4 upperLeft  = imat * vec4(-size.x * 0.5f, size.y * 0.5f, 0.f, 1.f);
    vec4 lowerLeft  = imat * vec4(-size.x * 0.5f, -size.y * 0.5f, 0.f, 1.f);
    vec4 lowerRight = imat * vec4(size.x * 0.5f, -size.y * 0.5f, 0.f, 1.f);
    vec4 upperRight = imat * vec4(size.x * 0.5f, size.y * 0.5f, 0.f, 1.f);

    // first triangle, lower left, starting in the upper left corner, counterclockwise
    m_Position.reserve(6);
    m_TexCoords.reserve(6);

    imat = glm::translate(center) * glm::rotate(glm::pi<float>() * -0.5f, glm::vec3(1.f, 0.f, 0.f));

    m_Position.emplace_back(imat * upperLeft);
    m_TexCoords.emplace_back(t0.x, !_flipHori ? t1.y : 1 - t0.y);

    m_Position.emplace_back(imat * lowerLeft);
    m_TexCoords.emplace_back(t0.x, !_flipHori ? t0.y : 1 - t1.y);

    m_Position.emplace_back(imat * lowerRight);
    m_TexCoords.emplace_back(t1.x, !_flipHori ? t0.y : 1 - t1.y);

    // second triangle, right upper, starting in the lower right corner, counterclockwise
    m_Position.emplace_back(imat * lowerRight);
    m_TexCoords.emplace_back(t1.x, !_flipHori ? t0.y : 1 - t1.y);

    m_Position.emplace_back(imat * upperRight);
    m_TexCoords.emplace_back(t1.x, !_flipHori ? t1.y : 1 - t0.y);

    m_Position.emplace_back(imat * upperLeft);
    m_TexCoords.emplace_back(t0.x, !_flipHori ? t1.y : 1 - t0.y);

    init();
}

Quad_P::Set &Quad_P::CreateCubeMap(Set &dest, glm::vec3 center, float cube_size, glm::vec4 color) {
    float ang[6][2] = {{0.f, 0.f}, {1.0f, 0.f}, {0.5f, 1.5f}, {0.5f, 0.0f}, {0.5f, 0.5f}, {0.5f, 1.0f}};
    float tex[6][4] = {{1, 0, 2, 1}, {1, 2, 2, 3}, {0, 1, 1, 2}, {1, 1, 2, 2}, {2, 1, 3, 2}, {3, 1, 4, 2}};

    for (int i = 0; i < 6; i++) {
        dest.emplace_back(make_unique<Quad_P>(center,
                                              vec2{cube_size, cube_size},
                                              cube_size * 0.5f,
                                              vec3(glm::pi<float>() * ang[i][0], glm::pi<float>() * ang[i][1], 0.f),
                                              vec2{tex[i][0] / 4.0f, tex[i][1] / 3.0f},
                                              vec2{tex[i][2] / 4.0f, tex[i][3] / 3.0f},
                                              true, color));
    }

    return dest;
}

void Quad_P::init() {
    m_mesh = make_unique<Mesh>("position:3f,normal:3f,texCoord:2f,color:4f");

    // move the quad to the correct position, in relation to the dimensions
    for (unsigned short i = 0; i < 6; i++) {
        m_mesh->push_back_positions(glm::value_ptr(m_Position[i]), 3);
        m_mesh->push_back_normals(glm::value_ptr(m_Normal), 3);
        m_mesh->push_back_texCoords(glm::value_ptr(m_TexCoords[i]), 2);
    }

    GLenum usage = GL_DYNAMIC_DRAW;

    m_vao = make_unique<VAO>("position:3f,normal:3f,texCoord:2f,color:4f", usage, nullptr, 1);
    m_vao->setStaticColor(m_color);
    m_vao->uploadMesh(m_mesh.get());
}

int Quad_P::getObjId() const {
    return objId;
}

int Quad_P::setObjId(int id) {
    objId = id;
    return objId;
}

int Quad_P::Set::GetIndexByObjId(int objid) {
    for (int i = 0; i < static_cast<int>(size()); i++) {
        if (at(i)->getObjId() == objid) {
            return i;
        }
    }

    return -1;
}

}  // namespace ara