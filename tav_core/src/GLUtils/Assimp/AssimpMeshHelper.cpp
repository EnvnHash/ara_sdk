//
//  AssimpMeshHelper.cpp
//  adapted from ofxAssimpMeshHelper.cpp
//  Created by Lukasz Karluk on 4/12/12.
//

#include "pch.h"
#include "AssimpMeshHelper.h"

namespace tav
{

AssimpMeshHelper::AssimpMeshHelper() : vao(NULL) {

	mesh = NULL;
	blendMode = BLENDMODE_ALPHA;
	twoSided = false;
	hasChanged = false;
}

//--------------------------------------------------------------

bool AssimpMeshHelper::hasTexture() {

	return static_cast<int>(textures.size()) > 0;
}

//--------------------------------------------------------------

std::vector<TextureManager*>* AssimpMeshHelper::getTextureRef() {

	return &textures;
}

//--------------------------------------------------------------

AssimpMeshHelper::~AssimpMeshHelper(){

	delete mesh;
	delete vao;
	for (std::vector< TextureManager* >::iterator it = textures.begin() ; it != textures.end(); ++it)
	    delete (*it);
}

}
