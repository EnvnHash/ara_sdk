
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
