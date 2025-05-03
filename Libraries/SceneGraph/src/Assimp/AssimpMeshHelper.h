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

    bool                   hasTexture() const { return !textures.empty(); }
    std::vector<std::unique_ptr<Texture>> & getTextureRef() { return textures; }

    std::unique_ptr<aiMesh>                 mesh;
    std::unique_ptr<VAO>                    vao;
    std::vector<std::unique_ptr<Texture>>   textures;
    std::vector<GLuint>                     indices;
    MaterialProperties                      material{};
    blendMode                               blendMode = BLENDMODE_ALPHA;
    bool                                    twoSided = false;
    bool                                    hasChanged = false;
    std::vector<aiVector3D>                 animatedPos;
    std::vector<aiVector3D>                 animatedNorm;
    glm::mat4                               matrix{};
    glm::vec3                               scaling{};
    glm::vec3                               translation{};
};
}  // namespace ara

#endif