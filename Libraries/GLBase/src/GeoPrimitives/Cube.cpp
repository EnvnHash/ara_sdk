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

#include "GeoPrimitives/Cube.h"

#include <Meshes/Mesh.h>
#include <Utils/VAO.h>

#define USE_PRIMITIVE_RESTART 1

using namespace std;
using namespace glm;

namespace ara {

Cube::Cube(float width, float height, float depth) {
    // A four vertices
    GLfloat cube_positions[] = {-width, -height, -depth, 1.0f, -width, -height, depth, 1.0f,
                                -width, height,  -depth, 1.0f, -width, height,  depth, 1.0f,
                                width,  -height, -depth, 1.0f, width,  -height, depth, 1.0f,
                                width,  height,  -depth, 1.0f, width,  height,  depth, 1.0f};

    glm::vec3 normal         = glm::normalize(glm::vec3(1.f, 1.f, 1.f));
    GLfloat   cube_normals[] = {-normal.x, -normal.y, -normal.z, 0.0f, -normal.x, -normal.y, normal.z, 0.0f,
                                -normal.x, normal.y,  -normal.z, 0.0f, -normal.x, normal.y,  normal.z, 0.0f,
                                normal.x,  -normal.y, -normal.z, 0.0f, normal.x,  -normal.y, normal.z, 0.0f,
                                normal.x,  normal.y,  -normal.z, 0.0f, normal.x,  normal.y,  normal.z, 0.0f};

    // Color for each vertex
    GLfloat cube_colors[] = {1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
                             1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f};

    // Indices for the triangle strips
    GLuint cube_indices[] = {
        0,      1, 2, 3, 6, 7, 4, 5,  // First strip
        0xFFFF,                       // <<- - This is the restart index
        2,      6, 0, 4, 1, 5, 3, 7   // Second strip
    };

    // Set
    m_vao = make_unique<VAO>("position:4f,normal:4f,color:4f", GL_STATIC_DRAW, nullptr, 1, true);
    m_vao->upload(CoordType::Position, cube_positions, 8);
    m_vao->upload(CoordType::Normal, cube_normals, 8);
    m_vao->upload(CoordType::Color, cube_colors, 8);
    m_vao->setElemIndices(19, cube_indices);
}

void Cube::draw(TFO *_tfo) {
    m_vao->drawElements(GL_TRIANGLE_STRIP, nullptr, GL_TRIANGLES, 8);
    m_vao->drawElements(GL_TRIANGLE_STRIP, nullptr, GL_TRIANGLES, 8, 9);
}

}  // namespace ara
