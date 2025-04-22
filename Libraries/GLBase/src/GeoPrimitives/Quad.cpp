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
    width  = 1.f;
    height = 1.f;

    instAttribs    = nullptr;
    maxNrInstances = 1;

    m_r = 1.0f;
    m_g = 1.0f;
    m_b = 1.0f;
    m_a = 1.0f;

    // first triangle, lower left, starting in the upper left corner,
    // counterclockwise
    position.reserve(6);
    texCoords.reserve(6);
    normal.reserve(6);

    position.push_back(glm::vec3(-0.5f, 0.5f, 0.f));
    texCoords.push_back(glm::vec2(0.f, 1.f));

    position.push_back(glm::vec3(-0.5f, -0.5f, 0.f));
    texCoords.push_back(glm::vec2(0.f, 0.f));

    position.push_back(glm::vec3(0.5f, -0.5f, 0.f));
    texCoords.push_back(glm::vec2(1.f, 0.f));

    // second triangle, right upper, starting in the lower right corner,
    // counterclockwise
    position.push_back(glm::vec3(0.5f, -0.5f, 0.f));
    texCoords.push_back(glm::vec2(0.f, 0.f));

    position.push_back(glm::vec3(0.5f, 0.5f, 0.f));
    texCoords.push_back(glm::vec2(1.f, 1.f));

    position.push_back(glm::vec3(-0.5f, 0.5f, 0.f));
    texCoords.push_back(glm::vec2(0.f, 1.f));

    // set normal facing outwards of the screen
    for (auto i = 0; i < 6; i++) normal.push_back(glm::vec3(0.f, 0.f, 1.f));

    init();
}

Quad::Quad(float x, float y, float w, float h, glm::vec3 inNormal, float _r, float _g, float _b, float _a,
           std::vector<CoordType> *_instAttribs, int _nrInstances, bool _flipHori)
    : GeoPrimitive() {
    width  = w;
    height = h;

    instAttribs    = _instAttribs;
    maxNrInstances = _nrInstances;

    m_r = _r;
    m_g = _g;
    m_b = _b;
    m_a = _a;

    glm::vec3 upperLeft  = glm::vec3(x, y + h, 0.f);
    glm::vec3 lowerLeft  = glm::vec3(x, y, 0.f);
    glm::vec3 lowerRight = glm::vec3(x + w, y, 0.f);
    glm::vec3 upperRight = glm::vec3(x + w, y + h, 0.f);

    // first triangle, lower left, starting in the upper left corner,
    // counterclockwise
    position.reserve(6);
    texCoords.reserve(6);
    normal.reserve(6);

    position.push_back(upperLeft);
    texCoords.push_back(glm::vec2(0.f, !_flipHori ? 1.f : 0.f));

    position.push_back(lowerLeft);
    texCoords.push_back(glm::vec2(0.f, !_flipHori ? 0.f : 1.f));

    position.push_back(lowerRight);
    texCoords.push_back(glm::vec2(1.f, !_flipHori ? 0.f : 1.f));

    // second triangle, right upper, starting in the lower right corner,
    // counterclockwise
    position.push_back(lowerRight);
    texCoords.push_back(glm::vec2(1.f, !_flipHori ? 0.f : 1.f));

    position.push_back(upperRight);
    texCoords.push_back(glm::vec2(1.f, !_flipHori ? 1.f : 0.f));

    position.push_back(upperLeft);
    texCoords.push_back(glm::vec2(0.f, !_flipHori ? 1.f : 0.f));

    // set normal facing outwards of the screen
    for (auto i = 0; i < 6; i++) normal.push_back(inNormal);

    init();
}

void Quad::init() {
    Mesh m("position:3f,normal:3f,texCoord:2f,color:4f");

    // move the quad to the correct position, in relation to the dimensions
    for (unsigned short i = 0; i < 6; i++) {
        GLfloat v[3] = {position[i].x, position[i].y, position[i].z};
        m.push_back_positions(v, 3);

        GLfloat n[3] = {normal[i].x, normal[i].y, normal[i].z};
        m.push_back_normals(n, 3);

        GLfloat t[2] = {texCoords[i].x, texCoords[i].y};
        m.push_back_texCoords(t, 2);
    }

    GLenum usage = GL_DYNAMIC_DRAW;
    if (instAttribs) usage = GL_DYNAMIC_DRAW;

    m_vao = make_unique<VAO>("position:3f,normal:3f,texCoord:2f,color:4f", usage, instAttribs, maxNrInstances);
    m_vao->setStaticColor(m_r, m_g, m_b, m_a);
    m_vao->uploadMesh(&m);
}

std::vector<glm::vec3> *Quad::getPositions() { return &position; }

std::vector<glm::vec3> *Quad::getNormals() { return &normal; }

std::vector<glm::vec2> *Quad::getTexCoords() { return &texCoords; }

}  // namespace ara
