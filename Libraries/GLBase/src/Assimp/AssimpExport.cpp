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
/**
 *	Mesh exporting with Assimp Library
 *	Note: Collada exporting is terribly slow, ...no way to optimize without
 *making changes inside the libray
 */

#ifdef ARA_USE_ASSIMP

#include "Assimp/AssimpExport.h"

#include <assimp/scene.h>

#include "Shaders/ShaderUtils/ShaderBuffer.h"

using namespace std;
using namespace Assimp;

namespace ara {
void AssimpExport::singleMeshExport(uint32_t nrVert, uint32_t nrIndices, ShaderBuffer<custVec3> *pos,
                                    ShaderBuffer<custVec3> *norm, ShaderBuffer<custVec3> *texc,
                                    ShaderBuffer<uint32_t> *indices, const string &path, const string &formatId) {
    auto scene = std::make_unique<aiScene>();

    scene->mRootNode = new aiNode();

    scene->mMaterials    = new aiMaterial *[1];
    scene->mMaterials[0] = nullptr;
    scene->mNumMaterials = 1;

    scene->mMaterials[0] = new aiMaterial();

    scene->mMeshes    = new aiMesh *[1];
    scene->mMeshes[0] = nullptr;
    scene->mNumMeshes = 1;

    scene->mMeshes[0]                 = new aiMesh();
    scene->mMeshes[0]->mMaterialIndex = 0;

    scene->mRootNode->mMeshes    = new unsigned int[1];
    scene->mRootNode->mMeshes[0] = 0;
    scene->mRootNode->mNumMeshes = 1;

    auto pMesh          = scene->mMeshes[0];
    pMesh->mVertices    = new aiVector3D[nrVert];
    pMesh->mNormals     = new aiVector3D[nrVert];
    pMesh->mNumVertices = nrVert;

    pMesh->mTextureCoords[0]   = new aiVector3D[nrVert];
    pMesh->mNumUVComponents[0] = nrVert;

    // map vertices and copy to aiScene
    // unfortunately there is no possibility to have a vec3 as a shader storage buffer could be further optimized by
    // using PBOs and async download, storages buffer would have to be copied on gpu to PBO
    auto posPtr = pos->map(GL_MAP_READ_BIT);
    std::copy_n(reinterpret_cast<uint8_t*>(posPtr), nrVert * sizeof(custVec3), reinterpret_cast<uint8_t*>(pMesh->mVertices));
    pos->unmap();

    auto normPtr = norm->map(GL_MAP_READ_BIT);
    std::copy_n(reinterpret_cast<uint8_t*>(normPtr), nrVert * sizeof(custVec3), reinterpret_cast<uint8_t*>(pMesh->mNormals));
    norm->unmap();

    auto texPtr = texc->map(GL_MAP_READ_BIT);
    std::copy_n(reinterpret_cast<uint8_t*>(texPtr), nrVert * sizeof(custVec3), reinterpret_cast<uint8_t*>(pMesh->mTextureCoords[0]));
    texc->unmap();

    // create faces (=triangles in this case), each face has 3 vertices
    pMesh->mFaces    = new aiFace[nrIndices / 3];
    pMesh->mNumFaces = nrIndices / 3;

    // map and copy indices
    // could be optimized, storage buffer should be [ind[0], ind[1], ind[2],
    // mNumIndices, ind[3], ind[4], ind[5]... this way would be a single std::copy
    uint32_t *indPtr = indices->map(GL_MAP_READ_BIT);
    int       k      = 0;
    for (uint32_t i = 0; i < (nrIndices / 3); i++, k += 3) {
        aiFace &face     = pMesh->mFaces[i];
        face.mIndices    = new unsigned int[3];
        face.mNumIndices = 3;
        std::copy_n( &indPtr[k], 3, face.mIndices);
    }
    indices->unmap();

    auto expProps = std::make_unique<ExportProperties>();
    expProps->SetPropertyBool("GLOB_MEASURE_TIME", true);

    exp.Export(scene.get(), formatId, path, 0, expProps.get());
}
}  // namespace ara

#endif