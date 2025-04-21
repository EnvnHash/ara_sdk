//
//  AssimpMeshHelper.cpp
//

#ifdef ARA_USE_ASSIMP

#include "AssimpMeshHelper.h"
#include <Meshes/Mesh.h>
#include <Utils/Texture.h>
#include <Utils/VAO.h>
#include <assimp/postprocess.h>

using namespace glm;
using namespace std;

namespace ara {

AssimpMeshHelper::AssimpMeshHelper() : vao(nullptr) {
    mesh       = nullptr;
    blendMode  = BLENDMODE_ALPHA;
    twoSided   = false;
    hasChanged = false;
}

bool AssimpMeshHelper::hasTexture() { return static_cast<int>(textures.size()) > 0; }

std::vector<Texture*>* AssimpMeshHelper::getTextureRef() { return &textures; }

AssimpMeshHelper::~AssimpMeshHelper() {
    if (mesh) delete mesh;
    if (vao) delete vao;
    for (std::vector<Texture*>::iterator it = textures.begin(); it != textures.end(); ++it) delete (*it);
}

}  // namespace ara
#endif