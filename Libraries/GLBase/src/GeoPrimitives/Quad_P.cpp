//
//  Quad.cpp
//
//  Created by Sven Hahne on 20.08.14.
//
//  creates a quad consisting of two triangles
//  x, y means the lower left corner

#include "GeoPrimitives/Quad_P.h"

#include <Meshes/Mesh.h>
#include <Utils/VAO.h>

using namespace glm;
using namespace std;

namespace ara {

Quad_P::Quad_P(glm::vec3 center, float w, float h, float dist, glm::vec3 euler, float tx0, float ty0, float tx1,
               float ty1, bool _flipHori, glm::vec4 color)
    : GeoPrimitive() {
    m_Size = vec2(w, h);

    mat4 imat = glm::eulerAngleXYZ(euler.x, euler.y, euler.z);
    m_Normal  = imat * vec4(0, 0, 1, 1);
    imat      = glm::translate(imat, vec3(0, 0, 0 + dist));

    m_Color = color;

    vec4 upperLeft  = imat * vec4(-w * 0.5f, h * 0.5f, 0.f, 1.f);
    vec4 lowerLeft  = imat * vec4(-w * 0.5f, -h * 0.5f, 0.f, 1.f);
    vec4 lowerRight = imat * vec4(w * 0.5f, -h * 0.5f, 0.f, 1.f);
    vec4 upperRight = imat * vec4(w * 0.5f, h * 0.5f, 0.f, 1.f);

    // first triangle, lower left, starting in the upper left corner,
    // counterclockwise
    m_Position.reserve(6);
    m_TexCoords.reserve(6);

    imat = glm::translate(center) * glm::rotate(glm::pi<float>() * -0.5f, glm::vec3(1.f, 0.f, 0.f));

    m_Position.emplace_back(imat * upperLeft);
    m_TexCoords.emplace_back(glm::vec2(tx0, !_flipHori ? ty1 : 1 - ty0));

    m_Position.emplace_back(imat * lowerLeft);
    m_TexCoords.emplace_back(glm::vec2(tx0, !_flipHori ? ty0 : 1 - ty1));

    m_Position.emplace_back(imat * lowerRight);
    m_TexCoords.emplace_back(glm::vec2(tx1, !_flipHori ? ty0 : 1 - ty1));

    // second triangle, right upper, starting in the lower right corner,
    // counterclockwise
    m_Position.emplace_back(imat * lowerRight);
    m_TexCoords.emplace_back(glm::vec2(tx1, !_flipHori ? ty0 : 1 - ty1));

    m_Position.emplace_back(imat * upperRight);
    m_TexCoords.emplace_back(glm::vec2(tx1, !_flipHori ? ty1 : 1 - ty0));

    m_Position.emplace_back(imat * upperLeft);
    m_TexCoords.emplace_back(glm::vec2(tx0, !_flipHori ? ty1 : 1 - ty0));

    init();
}

Quad_P::Set &Quad_P::CreateCubeMap(Set &dest, glm::vec3 center, float cube_size, glm::vec4 color) {
    float ang[6][2] = {{0.f, 0.f}, {1.0f, 0.f}, {0.5f, 1.5f}, {0.5f, 0.0f}, {0.5f, 0.5f}, {0.5f, 1.0f}};
    float tex[6][4] = {{1, 0, 2, 1}, {1, 2, 2, 3}, {0, 1, 1, 2}, {1, 1, 2, 2}, {2, 1, 3, 2}, {3, 1, 4, 2}};

    for (int i = 0; i < 6; i++)

        dest.emplace_back(
            make_unique<Quad_P>(center, cube_size, cube_size, cube_size * 0.5f,
                                glm::vec3(glm::pi<float>() * ang[i][0], glm::pi<float>() * ang[i][1], 0.f),
                                tex[i][0] / 4.0f, tex[i][1] / 3.0f, tex[i][2] / 4.0f, tex[i][3] / 3.0f, true, color));

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
    m_vao->setStaticColor(m_Color[0], m_Color[1], m_Color[2], m_Color[3]);
    m_vao->uploadMesh(m_mesh.get());
}

int Quad_P::getObjId() { return objId; }

int Quad_P::setObjId(int id) {
    objId = id;
    return objId;
}

int Quad_P::Set::GetIndexByObjId(int objid) {
    for (int i = 0; i < (int)size(); i++)
        if (at(i)->getObjId() == objid) return i;

    return -1;
}

}  // namespace ara