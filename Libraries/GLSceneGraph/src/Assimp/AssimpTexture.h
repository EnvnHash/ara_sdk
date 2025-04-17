#ifdef ARA_USE_ASSIMP

#pragma once

#include <glsg_common/glsg_common.h>

namespace ara {

class Texture;
class GLBase;

class AssimpTexture {
public:
    AssimpTexture(GLBase* glbase);
    virtual ~AssimpTexture() = default;

    Texture*    getTextureRef();
    std::string getTexturePath();
    bool        hasTexture();

private:
    std::unique_ptr<Texture> m_texture;
    std::string              m_texturePath;
};

}  // namespace ara

#endif
