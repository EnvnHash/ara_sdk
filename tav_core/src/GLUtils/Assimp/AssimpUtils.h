/**
*  AssimpUtils.h
*
*  Created by Sven Hahne on 16/8/15.
*/

#pragma once
#ifndef tav_core_AssimpUtils_h
#define tav_core_AssimpUtils_h

#include "AssimpMeshHelper.h"

namespace tav
{
static inline glm::vec4 aiColorToGlmColor(const aiColor4D& c)
{
	return glm::vec4(c.r, c.g, c.b, c.a);
}

//--------------------------------------------------------------

static inline glm::vec4 aiColorToGlmColor(const aiColor3D& c)
{
	return glm::vec4(c.r, c.g, c.b, 1);
}

//--------------------------------------------------------------

static inline glm::vec3 aiVecToOfVec(const aiVector3D& v)
{
	return glm::vec3(v.x, v.y, v.z);
}

//--------------------------------------------------------------

static inline std::vector<glm::vec3> aiVecVecToGlmVecVec(
		const std::vector<aiVector3D>& v)
{
	std::vector<glm::vec3> glmV(v.size());

	if (sizeof(aiVector3D) == sizeof(glm::vec3))
	{
		memcpy(&glmV[0], &v[0], v.size() * sizeof(glm::vec3));
	}
	else
	{
		for (int i = 0; i < (int) v.size(); i++)
			glmV[i] = aiVecToOfVec(v[i]);
	}
	return glmV;
}

//--------------------------------------------------------------

static void aiMeshToTavMesh(const aiMesh* aim, Mesh* _mesh,
		AssimpMeshHelper* helper = NULL)
{
	// default to triangle mode
	//        ofm.setMode(OF_PRIMITIVE_TRIANGLES);

	// copy vertices
	for (int i = 0; i < (int) aim->mNumVertices; i++)
	{
		GLfloat pos[3] =
		{ aim->mVertices[i].x, aim->mVertices[i].y, aim->mVertices[i].z };
		_mesh->push_back_positions(&pos[0], 3);
	}

	/*

	 if(aim->HasNormals())
	 {
	 for (int i=0; i < (int)aim->mNumVertices;i++)
	 {
	 GLfloat normal[3] = { aim->mNormals[i].x, aim->mNormals[i].y, aim->mNormals[i].z };
	 _mesh->push_back_normals(&normal[0], 3);
	 }
	 }
	 
	 // aiVector3D * 	mTextureCoords [AI_MAX_NUMBER_OF_TEXTURECOORDS]
	 // just one for now
	 if(aim->GetNumUVChannels()>0)
	 {
	 for (int i=0; i < (int)aim->mNumVertices;i++)
	 {
	 if( helper != NULL && helper->texture->getWidth() > 0.0 )
	 {
	 GLfloat* texCoord = helper->texture->getCoordFromPercent(aim->mTextureCoords[0][i].x,
	 aim->mTextureCoords[0][i].y);
	 _mesh->push_back_texCoords(&texCoord[0], 2);
	 
	 } else
	 {
	 GLfloat texCoord[2] = { aim->mTextureCoords[0][i].x, aim->mTextureCoords[0][i].y };
	 _mesh->push_back_texCoords(&texCoord[0], 2);
	 }
	 }
	 }
	 
	 //aiColor4D * 	mColors [AI_MAX_NUMBER_OF_COLOR_SETS]
	 // just one for now
	 if(aim->GetNumColorChannels()>0)
	 {
	 for (int i=0; i < (int)aim->mNumVertices;i++)
	 {
	 glm::vec4 col = aiColorToGlmColor(aim->mColors[0][i]);
	 _mesh->push_back_colors(&col[0], 4);
	 }
	 }
	 
	 for (int i=0; i <(int) aim->mNumFaces;i++)
	 {
	 if(aim->mFaces[i].mNumIndices>3)
	 DEBUG_POST("non-triangular face found: model face # " + std::to_string(i) );
	 
	 _mesh->push_back_indices((GLuint*) &aim->mFaces[i].mIndices[0], aim->mFaces[i].mNumIndices);
	 
	 //for (int j=0; j<(int)aim->mFaces[i].mNumIndices; j++)
	 //ofm.addIndex(aim->mFaces[i].mIndices[j]);
	 }
	 
	 _mesh->setName(std::string(aim->mName.data));
	 _mesh->setMaterialId(aim->mMaterialIndex);
	 */
}

//--------------------------------------------------------------

static glm::mat4 aiMatrix4x4ToGlmMatrix4x4(const aiMatrix4x4& aim)
{
	glm::mat4 toMat4 = glm::mat4(1.f);

	toMat4[0][0] = aim.a1;
	toMat4[0][1] = aim.a2;
	toMat4[0][2] = aim.a3;
	toMat4[0][3] = aim.a4;

	toMat4[1][0] = aim.b1;
	toMat4[1][1] = aim.b2;
	toMat4[1][2] = aim.b3;
	toMat4[1][3] = aim.b4;

	toMat4[2][0] = aim.c1;
	toMat4[2][1] = aim.c2;
	toMat4[2][2] = aim.c3;
	toMat4[2][3] = aim.c4;

	toMat4[3][0] = aim.d1;
	toMat4[3][1] = aim.d2;
	toMat4[3][2] = aim.d3;
	toMat4[3][3] = aim.d4;

	return toMat4;
}
}

#endif
