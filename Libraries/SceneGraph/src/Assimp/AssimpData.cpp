#ifdef ARA_USE_ASSIMP

#include "Assimp/AssimpData.h"
#include "Utils/VAO.h"

using namespace glm;
using namespace std;

namespace ara {

void AssimpData::setTextures(const AssimpMeshHelper& mesh, Shaders* shdr) const {
    if (bUsingTextures && mesh.hasTexture()) {
        for (uint t = 0; t < static_cast<int>(mesh.textures.size()); t++) {
            glActiveTexture(GL_TEXTURE0 + t);
            shdr->setUniform1i("tex_diffuse" + to_string(t), static_cast<int>(t));
            mesh.textures[t]->bind();
        }
    }
}

void AssimpData::sendMeshMatToShdr(uint meshNr, Shaders* shdr) {
    if (loadSuccess) {
        auto& mesh = modelMeshes[std::max<uint>(static_cast<uint>(0),
                                               std::min<uint>(meshNr, static_cast<uint>(modelMeshes.size() - 1)))];
        if (bUsingMaterials) {
            mesh.material.sendToShader(shdr->getProgram());
        }
    }
}

void AssimpData::setCullFace(const AssimpMeshHelper& mesh) {
    if (mesh.twoSided) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

void AssimpData::setMaterials(AssimpMeshHelper& mesh, Shaders* shdr) const {
    if (bUsingMaterials) {
        mesh.material.sendToShader(shdr->getProgram());
    }
}

void AssimpData::setBlendFunc(const AssimpMeshHelper& mesh) {
    if (mesh.blendMode == AssimpMeshHelper::BLENDMODE_ALPHA) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else if (mesh.blendMode == AssimpMeshHelper::BLENDMODE_ADD) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
}

void AssimpData::drawByRenderType(const AssimpMeshHelper& mesh, GLenum renderType, uint nrInst) const {
    if (nrInst > 0) {
        if (drawIndexed) {
            mesh.vao->drawElements(renderType);
        } else {
            mesh.vao->draw(renderType);
        }
    } else {
        if (drawIndexed) {
            mesh.vao->drawElementsInst(renderType, nrInst, nullptr, renderType);
        } else {
            mesh.vao->drawInstanced(renderType, nrInst, nullptr, static_cast<float>(mesh.mesh->mNumVertices));
        }
    }
}

void AssimpData::drawMeshNoShdr(uint meshNr, GLenum renderType) const {
    if (loadSuccess) {
        const auto& mesh = modelMeshes[std::max<uint>(static_cast<uint>(0),
                                               std::min<uint>(meshNr, static_cast<uint>(modelMeshes.size() - 1)))];
        drawByRenderType(mesh, renderType);
    }
}

void AssimpData::draw(AssimpMeshHelper& mesh, Shaders* shdr, GLenum renderType, bool useTextures, uint nrInst) const {
    if (useTextures) {
        setTextures(mesh, shdr);
    }

    setMaterials(mesh, shdr);
    setCullFace(mesh);
    setBlendFunc(mesh);
    drawByRenderType(mesh, renderType);
}

void AssimpData::drawMesh(uint meshNr, GLenum renderType, Shaders* shdr) {
    if (glOk) {
        auto& mesh = modelMeshes[std::max<uint>(0, std::min<uint>(meshNr, static_cast<uint>(modelMeshes.size() - 1)))];
        draw(mesh, shdr, renderType, true);
    }
}

void AssimpData::draw(GLenum renderType, Shaders* shdr) {
    if (glOk) {
        for (auto& mesh : modelMeshes) {
            draw(mesh, shdr, renderType, true);
        }
    }
}

void AssimpData::drawInstanced(GLenum renderType, uint nrInst, Shaders* shdr)  {
    if (glOk) {
        for (auto& mesh : modelMeshes) {
            draw(mesh, shdr, renderType, true, nrInst);
        }
    }
}

void AssimpData::drawNoText(uint meshNr, GLenum renderType, Shaders* shdr) {
    if (glOk) {
        for (auto& mesh : modelMeshes) {
            draw(mesh, shdr, renderType, false);
        }
    }
}

void AssimpData::update(double time) {
    if (glOk) {
        if (!scene) {
            return;
        }
        updateAnimations(time);
        updateMeshes(scene->mRootNode, mat4(1.f));

        if (hasAnimations() == false) {
            return;
        }

        updateBones();
        updateGLResources();
    }
}

void AssimpData::updateAnimations(double time) {
    if (glOk) {
        for (auto & animation : animations) {
            animation.update(time);
        }
    }
}

void AssimpData::updateMeshes(const aiNode* node, const mat4 &parentMatrix) {
    if (glOk) {
        aiMatrix4x4 m = node->mTransformation;
        m.Transpose();
        mat4 matrix(m.a1, m.a2, m.a3, m.a4, m.b1, m.b2, m.b3, m.b4, m.c1, m.c2, m.c3, m.c4, m.d1, m.d2, m.d3, m.d4);
        matrix *= parentMatrix;

        for (uint i = 0; i < node->mNumMeshes; i++) {
            int   meshIndex = static_cast<int>(node->mMeshes[i]);
            auto& mesh      = modelMeshes[meshIndex];
            mesh.matrix     = matrix;
        }

        for (uint i = 0; i < node->mNumChildren; i++) {
            updateMeshes(node->mChildren[i], matrix);
        }
    }
}

void AssimpData::updateBones() {
    if (glOk) {
        // update mesh position for the animation
        for (auto& modelMesh : modelMeshes) {
            // current mesh we are introspecting
            const auto& mesh = modelMesh.mesh;

            // calculate bone matrices
            vector<aiMatrix4x4> boneMatrices(mesh->mNumBones);
            for (uint a = 0; a < mesh->mNumBones; ++a) {
                const aiBone* bone = mesh->mBones[a];

                // find the corresponding node by again looking recursively
                // through the node hierarchy for the same name
                aiNode* node = scene->mRootNode->FindNode(bone->mName);

                // start with the mesh-to-bone matrix
                boneMatrices[a] = bone->mOffsetMatrix;

                // and now append all node transformations down the parent chain
                // until we're back at mesh coordinates again
                const aiNode* tempNode = node;

                while (tempNode) {
                    // check your matrix multiplication order here!!!
                    boneMatrices[a] = tempNode->mTransformation * boneMatrices[a];
                    // boneMatrices[a] = boneMatrices[a] *
                    // tempNode->mTransformation;
                    tempNode = tempNode->mParent;
                }
                modelMesh.hasChanged = true;
            }

            modelMesh.animatedPos.assign(modelMesh.animatedPos.size(), aiVector3D(0.0f));

            if (mesh->HasNormals())
                modelMesh.animatedNorm.assign(modelMesh.animatedNorm.size(), aiVector3D(0.0f));

            // loop through all vertex weights of all bones
            for (uint a = 0; a < mesh->mNumBones; ++a) {
                const aiBone*      bone     = mesh->mBones[a];
                const aiMatrix4x4& posTrafo = boneMatrices[a];

                for (uint b = 0; b < bone->mNumWeights; ++b) {
                    const aiVertexWeight& weight = bone->mWeights[b];

                    size_t            vertexId = weight.mVertexId;
                    const aiVector3D& srcPos   = mesh->mVertices[vertexId];

                    modelMesh.animatedPos[vertexId] += weight.mWeight * (posTrafo * srcPos);
                }

                if (mesh->HasNormals()) {
                    // 3x3 matrix, contains the bone matrix without the
                    // translation, only with rotation and possibly scaling
                    auto normTrafo = aiMatrix3x3(posTrafo);
                    for (uint b = 0; b < bone->mNumWeights; ++b) {
                        const aiVertexWeight& weight   = bone->mWeights[b];
                        size_t                vertexId = weight.mVertexId;

                        const aiVector3D& srcNorm = mesh->mNormals[vertexId];
                        modelMesh.animatedNorm[vertexId] += weight.mWeight * (normTrafo * srcNorm);
                    }
                }
            }
        }
    }
}

void AssimpData::updateGLResources() {
    /*
            if (glOk)
            {
                     // now upload the result position and normal along with the
       other vertex attributes into a dynamic vertex buffer, VBO or whatever for
       (uint i = 0; i < modelMeshes.size(); ++i)
                     {
                     if(modelMeshes[i].hasChanged)
                     {
                     const aiMesh* mesh = modelMeshes[i].mesh;
                     modelMeshes[i].vbo.updateVertexData(&modelMeshes[i].animatedPos[0].x,mesh->mNumVertices);

                     if(mesh->HasNormals())
                     modelMeshes[i].vbo.updateNormalData(&modelMeshes[i].animatedNorm[0].x,mesh->mNumVertices);

                     modelMeshes[i].hasChanged = false;
                     }
                     }
            }
    */
}

AssimpAnimation& AssimpData::getAnimation(int animationIndex) {
    animationIndex = static_cast<int>(glm::clamp(static_cast<float>(animationIndex), 0.f, static_cast<float>(animations.size() - 1)));
    return animations[animationIndex];
}

vector<string> AssimpData::getMeshNames() const {
    vector<string> names(scene->mNumMeshes);
    for (int i = 0; i < static_cast<int>(scene->mNumMeshes); i++) {
        names[i] = scene->mMeshes[i]->mName.data;
    }
    return names;
}

[[maybe_unused]] Mesh* AssimpData::getMesh(const string& name) const {
    aiMesh* aim = nullptr;

    for (int i = 0; i < static_cast<int>(scene->mNumMeshes); i++) {
        if (string(scene->mMeshes[i]->mName.data) == name) {
            aim = scene->mMeshes[i];
            break;
        }
    }

    if (!aim) {
        LOGE << "couldn't find mesh " << name.c_str();
        return nullptr;
    } else {
        Mesh* mesh = nullptr;
        aiMeshToTavMesh(aim, mesh);
        return mesh;
    }
}

Mesh* AssimpData::getMesh(int num) const {
    if (static_cast<int>(scene->mNumMeshes) <= num) {
        LOGE << "couldn't find mesh " << num << " there's only " << scene->mNumMeshes;
        return nullptr;
    } else {
        Mesh* mesh = nullptr;
        aiMeshToTavMesh(scene->mMeshes[num], mesh);
        return mesh;
    }
}

VAO* AssimpData::getVao(int num) const {
    if (static_cast<int>(scene->mNumMeshes) <= num) {
        LOGE << "couldn't find mesh " << num << " there's only " << scene->mNumMeshes;
        return nullptr;
    } else {
        return modelMeshes[num].vao.get();
    }
}

[[maybe_unused]] Mesh* AssimpData::getCurrentAnimatedMesh(const string& name) {
    Mesh* ptr   = nullptr;
    bool  found = false;

    for (const auto & modelMeshe : modelMeshes) {
        if (string(modelMeshe.mesh->mName.data) == name) {
            /*
             if(!modelMeshes[i]->validCache)
             {
             modelMeshes[i]->cachedMesh.clear_positions();
             modelMeshes[i]->cachedMesh.clear_normals();

             vector<vec3> pos =
             aiVecVecToGlmVecVec(modelMeshes[i]->animatedPos); for(short
             j=0;j<(int)pos.size();j++)
             modelMeshes[i]->cachedMesh.emplace_back_positions(&pos[j][0], 3);

             vector<vec3> norm =
             aiVecVecToGlmVecVec(modelMeshes[i]->animatedNorm); for(short
             j=0;j<(int)pos.size();j++)
             modelMeshes[i]->cachedMesh.push_back_normals(&norm[j][0], 3);

             modelMeshes[i]->validCache = true;

             found = true;
             }
             ptr = &modelMeshes[i]->cachedMesh;
             */
        }
    }

    if (!found) {
        LOGE << "couldn't find mesh " << name;
    }
    return ptr;
}
[[maybe_unused]] Mesh* AssimpData::getCurrentAnimatedMesh(int num) const {
    Mesh* ptr = nullptr;

    if (static_cast<int>(modelMeshes.size()) <= num) {
        LOG << "couldn't find mesh " << num << " there's only " << scene->mNumMeshes;
    } else {
        /*
         if(!modelMeshes[num]->validCache)
         {
         modelMeshes[num]->cachedMesh.clear_positions();
         modelMeshes[num]->cachedMesh.clear_normals();

         vector<vec3> pos = aiVecVecToGlmVecVec(modelMeshes[num]->animatedPos);
         for(short i=0;i<(int)pos.size();i++)
         modelMeshes[i]->cachedMesh.push_back_positions(&pos[i][0], 3);

         vector<vec3> norm =
         aiVecVecToGlmVecVec(modelMeshes[num]->animatedNorm); for(short
         i=0;i<(int)pos.size();i++)
         modelMeshes[i]->cachedMesh.push_back_normals(&norm[i][0], 3);

         modelMeshes[num]->validCache = true;

         }
         ptr = &modelMeshes[num]->cachedMesh;
         */
    }

    return ptr;
}

const MaterialProperties *AssimpData::getMaterialForMesh(const string &name) const {
    for (const auto & modelMesh : modelMeshes) {
        if (string(modelMesh.mesh->mName.data) == name) {
            return &modelMesh.material;
        }
    }

    LOGE << "couldn't find mesh " << name;
    return nullptr;
}

MaterialProperties* AssimpData::getMaterialForMesh(int num)  {
    if (static_cast<int>(modelMeshes.size()) <= num) {
        LOGE << "couldn't find mesh " << num << " there's only " << scene->mNumMeshes;
        return nullptr;
    }
    return &modelMeshes[num].material;
}

Texture* AssimpData::getTextureForMesh(const string& name, int ind) const {
    auto r = ranges::find_if(modelMeshes, [&](auto &it) { return string(it.mesh->mName.data) == name; });

    if (r == modelMeshes.end()) {
        LOGE << "couldn't find mesh " << name;
        return nullptr;
    } else {
        return r->textures[ind].get();
    }

}

Texture* AssimpData::getTextureForMesh(int num, int ind) const {
    if (static_cast<int>(modelMeshes.size()) <= num) {
        LOGE << "couldn't find mesh " << num << " there's only " << scene->mNumMeshes;
        return nullptr;
    }
    return modelMeshes[num].textures[ind].get();
}

vec3 AssimpData::getDimensions() const {
    return vec3(scene_max.x, scene_max.y, scene_max.z) - vec3(scene_min.x, scene_min.y, scene_min.z);
}

vec3 AssimpData::getSceneMin(bool bScaled) const {
    vec3 sceneMin(scene_min.x, scene_min.y, scene_min.z);
    return bScaled ? sceneMin * scale : sceneMin;
}

vec3 AssimpData::getSceneMax(bool bScaled) const {
    vec3 sceneMax(scene_max.x, scene_max.y, scene_max.z);
     return bScaled ? sceneMax * scale : sceneMax;
}

vec3 AssimpData::getRotationAxis(int which) const {
    return static_cast<uint>(rotAxis.size()) > static_cast<uint>(which) ? rotAxis[which] : vec3{};
}

float AssimpData::getRotationAngle(int which) const {
    return (static_cast<uint>(rotAngle.size()) > static_cast<uint>(which)) ? rotAngle[which] : 0.f;
}

}  // namespace ara

#endif