//
//  AssimpUtils.h
//
//  Created by Sven Hahne on 16/8/15.
//
//

#pragma once

#ifdef ARA_USE_ASSIMP

#include "AssimpMeshHelper.h"
#include <assimp/mesh.h>
#include <Meshes/Mesh.h>

namespace ara {

static glm::vec4 aiColorToGlmColor(const aiColor4D& c) { return {c.r, c.g, c.b, c.a}; }
static glm::vec4 aiColorToGlmColor(const aiColor3D& c) { return {c.r, c.g, c.b, 1}; }
static glm::vec3 aiVecToOfVec(const aiVector3D& v) { return {v.x, v.y, v.z}; }

static std::vector<glm::vec3> aiVecVecToGlmVecVec(const std::vector<aiVector3D>& v) {
    std::vector<glm::vec3> glmV(v.size());

    if (sizeof(aiVector3D) == sizeof(glm::vec3)) {
        std::copy(&v[0].x, &v[0].x + v.size() * 3, &glmV[0].r);
    } else {
        for (int i = 0; i < static_cast<int>(v.size()); i++) glmV[i] = aiVecToOfVec(v[i]);
    }
    return glmV;
}

static void aiMeshToTavMesh(const aiMesh* aim, Mesh* mesh, AssimpMeshHelper* helper = nullptr) {
    // copy vertices
    for (int i = 0; i < static_cast<int>(aim->mNumVertices); i++) {
        GLfloat pos[3] = {aim->mVertices[i].x, aim->mVertices[i].y, aim->mVertices[i].z};
        mesh->push_back_positions(&pos[0], 3);
    }
}

static glm::mat4 aiMatrix4x4ToGlmMatrix4x4(const aiMatrix4x4& aim) {
    glm::mat4 toMat4 = glm::mat4(1.f);

    toMat4[0][0] = aim.a1;
    toMat4[0][1] = aim.a2;
    toMat4[0][2] = aim.a3;
    toMat4[0][3] = aim.a4;

    toMat4[1][0] = aim.b1;
    toMat4[1][1] = aim.b2;
    toMat4[1][2] = aim.b3;
    toMat4[1][3] = aim.b4;

    toMat4[2][0] = aim.c1;
    toMat4[2][1] = aim.c2;
    toMat4[2][2] = aim.c3;
    toMat4[2][3] = aim.c4;

    toMat4[3][0] = aim.d1;
    toMat4[3][1] = aim.d2;
    toMat4[3][2] = aim.d3;
    toMat4[3][3] = aim.d4;

    return toMat4;
}
}  // namespace ara

#endif