//
//  AssimpTexture.cpp
//
//  Created by Sven Hahne on 16/8/15.
//

#include "pch.h"
#include "AssimpTexture.h"

namespace tav
{
AssimpTexture::AssimpTexture()
{
	texturePath = "";
}

//--------------------------------------------------------------

AssimpTexture::AssimpTexture(TextureManager* texture, std::string texturePath)
{
	this->texture = *texture;
	this->texturePath = texturePath;
}

//--------------------------------------------------------------

TextureManager* AssimpTexture::getTextureRef()
{
	return &texture;
}

//--------------------------------------------------------------

std::string AssimpTexture::getTexturePath()
{
	return texturePath;
}

//--------------------------------------------------------------

bool AssimpTexture::hasTexture()
{
	return texture.isAllocated();
}
}
