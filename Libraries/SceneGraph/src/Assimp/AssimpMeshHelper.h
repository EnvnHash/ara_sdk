#pragma once

#ifdef ARA_USE_ASSIMP

#include <Shaders/ShaderUtils/MaterialProperties.h>
#include <assimp/mesh.h>
#include <assimp/vector3.h>
#include <glsg_common/glsg_common.h>

namespace ara {

class Texture;
class VAO;

class AssimpMeshHelper {
public:
    enum blendMode { BLENDMODE_ALPHA, BLENDMODE_ADD };
    enum meshMode { PRIMITIVE_TRIANGLES };

    AssimpMeshHelper();
    virtual ~AssimpMeshHelper();

    bool                   hasTexture();
    std::vector<Texture*>* getTextureRef();

    aiMesh* mesh;  // pointer to the aiMesh we represent.
    VAO*    vao;

    // texture
    std::vector<Texture*> textures;
    std::vector<GLuint>   indices;

    // Material
    MaterialProperties material;
    blendMode          blendMode;

    bool twoSided;
    bool hasChanged;

    std::vector<aiVector3D> animatedPos;
    std::vector<aiVector3D> animatedNorm;

    glm::mat4 matrix;
    glm::vec3 scaling;
    glm::vec3 translation;
};
}  // namespace ara

#endif