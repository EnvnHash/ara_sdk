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
    std::array<vec4, 8> cube_positions = {
        vec4{-width, -height, -depth, 1.0f}, vec4{-width, -height, depth, 1.0f},
        vec4{-width, height,  -depth, 1.0f}, vec4{-width, height,  depth, 1.0f},
        vec4{width,  -height, -depth, 1.0f}, vec4{width,  -height, depth, 1.0f},
        vec4{width,  height,  -depth, 1.0f}, vec4{width,  height,  depth, 1.0f}};

    vec3 normal         = normalize(vec3(1.f, 1.f, 1.f));

    std::array<vec4, 8> cube_normals = {
        vec4{-normal.x, -normal.y, -normal.z, 0.f}, vec4{-normal.x, -normal.y, normal.z, 0.f},
        vec4{-normal.x, normal.y,  -normal.z, 0.f}, vec4{-normal.x, normal.y,  normal.z, 0.f},
        vec4{normal.x,  -normal.y, -normal.z, 0.f}, vec4{normal.x,  -normal.y, normal.z, 0.f},
        vec4{normal.x,  normal.y,  -normal.z, 0.f}, vec4{normal.x,  normal.y,  normal.z, 0.f}};

    // Color for each vertex
    auto white = vec4{1.f, 1.f, 1.f, 1.f};
    std::array<vec4, 8> cube_colors = { white, white, white, white, white, white, white, white};

    // Indices for the triangle strips
    std::array<GLuint, 17> cube_indices = {
        0,      1, 2, 3, 6, 7, 4, 5,  // First strip
        0xFFFF,                       // <<- - This is the restart index
        2,      6, 0, 4, 1, 5, 3, 7   // Second strip
    };

    // Set
    m_vao = make_unique<VAO>("position:4f,normal:4f,color:4f", GL_STATIC_DRAW, nullptr, 1, true);
    m_vao->upload(CoordType::Position, &cube_positions[0][0], cube_positions.size());
    m_vao->upload(CoordType::Normal, &cube_normals[0][0], cube_normals.size());
    m_vao->upload(CoordType::Color, &cube_colors[0][0], cube_colors.size());
    m_vao->setElemIndices(cube_indices.size(), cube_indices.data());
}

void Cube::draw(TFO* tfo) {
    m_vao->drawElements(GL_TRIANGLE_STRIP, nullptr, GL_TRIANGLES, 8);
    m_vao->drawElements(GL_TRIANGLE_STRIP, nullptr, GL_TRIANGLES, 8, 9);
}

}  // namespace ara
