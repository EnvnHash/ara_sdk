//
//  AssimpTexture.cpp
//
//  Created by Sven Hahne on 16/8/15.
//

#ifdef ARA_USE_ASSIMP

#include "AssimpTexture.h"
#include <GLBase.h>
#include <Utils/Texture.h>

using namespace std;

namespace ara {

AssimpTexture::AssimpTexture(GLBase* glbase) {
    m_texturePath = "";
    m_texture     = make_unique<Texture>(glbase);
}

/*
AssimpTexture::AssimpTexture(Texture* texture, std::string texturePath)
{
    m_texture = *texture;
    m_texturePath = texturePath;
}
*/

Texture* AssimpTexture::getTextureRef() { return m_texture.get(); }

std::string AssimpTexture::getTexturePath() { return m_texturePath; }

bool AssimpTexture::hasTexture() { return m_texture->isAllocated(); }

}  // namespace ara

#endif