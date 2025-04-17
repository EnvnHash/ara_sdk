//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  creates a MPQuad consisting of two triangles
//  x, y means the lower left corner

#include "Meshes/MPQuad.h"

using namespace glm;
using namespace std;

namespace ara {

MPQuad::MPQuad() {
    m_width  = 1.f;
    m_height = 1.f;
    m_r      = 1.f;
    m_g      = 1.f;
    m_b      = 1.f;
    m_a      = 1.f;

    // first triangle, lower left, starting in the upper left corner,
    // counterclockwise
    m_positions.push_back(-0.5f);
    m_positions.push_back(0.5f);
    m_positions.push_back(0.f);
    m_texCoords.push_back(0.f);
    m_texCoords.push_back(1.f);

    m_positions.push_back(-0.5f);
    m_positions.push_back(-0.5f);
    m_positions.push_back(0.f);
    m_texCoords.push_back(0.f);
    m_texCoords.push_back(0.f);

    m_positions.push_back(0.5f);
    m_positions.push_back(-0.5f);
    m_positions.push_back(0.f);
    m_texCoords.push_back(1.f);
    m_texCoords.push_back(0.f);

    // second triangle, right upper, starting in the lower right corner,
    // counterclockwise
    m_positions.push_back(0.5f);
    m_positions.push_back(-0.5f);
    m_positions.push_back(0.f);
    m_texCoords.push_back(0.f);
    m_texCoords.push_back(0.f);

    m_positions.push_back(0.5f);
    m_positions.push_back(0.5f);
    m_positions.push_back(0.f);
    m_texCoords.push_back(1.f);
    m_texCoords.push_back(1.f);

    m_positions.push_back(-0.5f);
    m_positions.push_back(0.5f);
    m_positions.push_back(0.f);
    m_texCoords.push_back(0.f);
    m_texCoords.push_back(1.f);

    // set normal facing outwards of the screen
    for (auto i = 0; i < 6; i++) {
        m_normals.push_back(0.f);
        m_normals.push_back(0.f);
        m_normals.push_back(1.f);
    }

    MPQuad::init();
}

MPQuad::MPQuad(float x, float y, float w, float h) {
    m_width  = w;
    m_height = h;
    m_r      = 1.f;
    m_g      = 1.f;
    m_b      = 1.f;
    m_a      = 1.f;

    vec3 upperLeft  = vec3(x, y + h, 0.f);
    vec3 lowerLeft  = vec3(x, y, 0.f);
    vec3 lowerRight = vec3(x + w, y, 0.f);
    vec3 upperRight = vec3(x + w, y + h, 0.f);

    // first triangle, lower left, starting in the upper left corner,
    // counterclockwise
    for (int i = 0; i < 3; i++) m_positions.push_back(upperLeft[i]);
    m_texCoords.push_back(0.f);
    m_texCoords.push_back(1.f);

    for (int i = 0; i < 3; i++) m_positions.push_back(lowerLeft[i]);
    m_texCoords.push_back(0.f);
    m_texCoords.push_back(0.f);

    for (int i = 0; i < 3; i++) m_positions.push_back(lowerRight[i]);
    m_texCoords.push_back(1.f);
    m_texCoords.push_back(0.f);

    // second triangle, right upper, starting in the lower right corner,
    // counterclockwise
    for (int i = 0; i < 3; i++) m_positions.push_back(lowerRight[i]);
    m_texCoords.push_back(1.f);
    m_texCoords.push_back(0.f);

    for (int i = 0; i < 3; i++) m_positions.push_back(upperRight[i]);
    m_texCoords.push_back(1.f);
    m_texCoords.push_back(1.f);

    for (int i = 0; i < 3; i++) m_positions.push_back(upperLeft[i]);
    m_texCoords.push_back(0.f);
    m_texCoords.push_back(1.f);

    // set normal facing outwards of the screen
    for (auto i = 0; i < 6; i++) {
        m_normals.push_back(0.f);
        m_normals.push_back(0.f);
        m_normals.push_back(1.f);
    }

    MPQuad::init();
}

MPQuad::MPQuad(float x, float y, float w, float h, glm::vec3 inNormal, float r, float g, float b, float a) {
    m_width  = w;
    m_height = h;

    m_r = r;
    m_g = g;
    m_b = b;
    m_a = a;

    vec3 upperLeft  = vec3(x, y + h, 0.f);
    vec3 lowerLeft  = vec3(x, y, 0.f);
    vec3 lowerRight = vec3(x + w, y, 0.f);
    vec3 upperRight = vec3(x + w, y + h, 0.f);

    // get the rotation matrix for the projection from inNormal to the standard
    // normal
    quat rot = RotationBetweenVectors(vec3(0.f, 0.f, 1.f), inNormal);
    mat4 rMatr;
    // special case if inNormal is opposite of standard normal
    if (inNormal != vec3(0.f, 0.f, -1.f))
        rMatr = glm::mat4_cast(rot);
    else
        rMatr = glm::rotate(mat4(1.f), static_cast<float>(M_PI), vec3(0.f, 1.f, 0.f));

    // apply the matrix
    glm::vec4 tempV;
    tempV     = (rMatr * vec4(upperLeft, 1.f));
    upperLeft = vec3(tempV.x, tempV.y, tempV.z);

    tempV     = rMatr * vec4(lowerLeft, 1.f);
    lowerLeft = vec3(tempV.x, tempV.y, tempV.z);

    tempV      = rMatr * vec4(lowerRight, 1.f);
    lowerRight = vec3(tempV.x, tempV.y, tempV.z);

    tempV      = rMatr * vec4(upperRight, 1.f);
    upperRight = vec3(tempV.x, tempV.y, tempV.z);

    // first triangle, lower left, starting in the upper left corner,
    // counterclockwise
    for (int i = 0; i < 3; i++) m_positions.push_back(upperLeft[i]);
    m_texCoords.push_back(0.f);
    m_texCoords.push_back(1.f);

    for (int i = 0; i < 3; i++) m_positions.push_back(lowerLeft[i]);
    m_texCoords.push_back(0.f);
    m_texCoords.push_back(0.f);

    for (int i = 0; i < 3; i++) m_positions.push_back(lowerRight[i]);
    m_texCoords.push_back(1.f);
    m_texCoords.push_back(0.f);

    // second triangle, right upper, starting in the lower right corner,
    // counterclockwise
    for (int i = 0; i < 3; i++) m_positions.push_back(lowerRight[i]);
    m_texCoords.push_back(1.f);
    m_texCoords.push_back(0.f);

    for (int i = 0; i < 3; i++) m_positions.push_back(upperRight[i]);
    m_texCoords.push_back(1.f);
    m_texCoords.push_back(1.f);

    for (int i = 0; i < 3; i++) m_positions.push_back(upperLeft[i]);
    m_texCoords.push_back(0.f);
    m_texCoords.push_back(1.f);

    // set normal facing outwards of the screen
    for (auto i = 0; i < 6; i++) {
        m_normals.push_back(0.f);
        m_normals.push_back(0.f);
        m_normals.push_back(1.f);
    }

    init();
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
