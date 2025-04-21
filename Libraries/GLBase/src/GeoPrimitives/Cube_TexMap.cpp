#include "GeoPrimitives/Cube_TexMap.h"

#include <Meshes/Mesh.h>

#include "Utils/VAO.h"

#define USE_PRIMITIVE_RESTART 1

using namespace std;

namespace ara {

Cube_TexMap::Cube_TexMap(float w, float h, float d) : GeoPrimitive() {
#define v(a, b, c) (a) * w, (b) * h, (c) * d, 1.0f

    GLfloat cube_positions[36 * 4] = {
        v(-1, -1, +1), v(+1, -1, +1), v(-1, +1, +1), v(+1, +1, +1), v(-1, +1, +1), v(+1, -1, +1),  // top
        v(-1, +1, +1), v(-1, +1, -1), v(-1, -1, +1), v(-1, -1, -1), v(-1, -1, +1), v(-1, +1, -1),  // left
        v(-1, -1, -1), v(+1, -1, -1), v(-1, -1, +1), v(+1, -1, +1), v(-1, -1, +1), v(+1, -1, -1),  // front
        v(-1, -1, -1), v(-1, +1, -1), v(+1, -1, -1), v(+1, +1, -1), v(+1, -1, -1), v(-1, +1, -1),  // bottom
        v(+1, -1, -1), v(+1, +1, -1), v(+1, -1, +1), v(+1, +1, +1), v(+1, -1, +1), v(+1, +1, -1),  // right
        v(+1, +1, -1), v(-1, +1, -1), v(+1, +1, +1), v(-1, +1, +1), v(+1, +1, +1), v(-1, +1, -1),  // back

    };

    GLfloat           cube_normals[36 * 3];
    constexpr GLfloat nvalue[6 * 3] = {0, 0, 1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 1, 0, 0, 0, 1, 0};

    for (int i = 0; i < 6; i++){
        for (int j = 0; j < 6; j++) {
            std::copy(&nvalue[i * 3], &nvalue[i * 3] + 3, &cube_normals[i * 6 * 3 + j * 3]);
        }
    }

#define p(a, b) (a) / 4.0f, 1 - (b) / 3.0f

    GLfloat tex_coords[36 * 2] = {
        p(1, 1), p(2, 1), p(1, 0), p(2, 0), p(1, 0), p(2, 1),  // top
        p(0, 1), p(0, 2), p(1, 1), p(1, 2), p(1, 1), p(0, 2),  // left
        p(1, 2), p(2, 2), p(1, 1), p(2, 1), p(1, 1), p(2, 2),  // front
        p(1, 2), p(1, 3), p(2, 2), p(2, 3), p(2, 2), p(1, 3),  // bottom
        p(2, 2), p(3, 2), p(2, 1), p(3, 1), p(2, 1), p(3, 2),  // right
        p(3, 2), p(4, 2), p(3, 1), p(4, 1), p(3, 1), p(4, 2),  // bottom
    };

    m_vao = make_unique<VAO>("position:4f,normal:3f,texCoord:2f", GL_STATIC_DRAW, nullptr, 1, true);
    m_vao->upload(CoordType::Position, cube_positions, 36);
    m_vao->upload(CoordType::Normal, cube_normals, 36);
    m_vao->upload(CoordType::TexCoord, tex_coords, 36);
}

void Cube_TexMap::draw(TFO *_tfo) {
    m_vao->draw(GL_TRIANGLES, 0, 36, nullptr);
}

void Cube_TexMap::init() {}

}  // namespace ara
