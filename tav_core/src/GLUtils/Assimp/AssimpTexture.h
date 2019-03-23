//
//  AssimpTexture.h
//
//  Created by Sven Hahne on 16/8/15.
//

#pragma once
#ifndef __tav_core__AssimpTexture__
#define __tav_core__AssimpTexture__

#include "GLUtils/TextureManager.h"

namespace tav
{
class AssimpTexture
{
public:
	AssimpTexture();
	AssimpTexture(TextureManager* texture, std::string texturePath);

	TextureManager* getTextureRef();
	std::string getTexturePath();
	bool hasTexture();

private:
	TextureManager texture;
	std::string texturePath;
};
}

#endif /* defined(__mc3d_core__AssimpTexture__) */
