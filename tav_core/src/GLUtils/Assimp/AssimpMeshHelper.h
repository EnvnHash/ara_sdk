#ifndef tav_core_AssimpMeshHelper_h
#define tav_core_AssimpMeshHelper_h

#include <glm/glm.hpp>

#include <assimp/config.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Meshes/Mesh.h"
#include "GLUtils/TextureManager.h"
#include "GLUtils/VAO.h"
#include "Shaders/ShaderUtils/MaterialProperties.h"

namespace tav
{
class AssimpMeshHelper
{
public:
	enum blendMode { BLENDMODE_ALPHA, BLENDMODE_ADD };
	enum meshMode {	PRIMITIVE_TRIANGLES };

	AssimpMeshHelper();
	~AssimpMeshHelper();

	bool hasTexture();
	std::vector<TextureManager*>* getTextureRef();

	aiMesh* mesh;   // pointer to the aiMesh we represent.
	VAO* vao;

	// texture
	std::vector<TextureManager*> textures;
	std::vector<GLuint> indices;

	// Material
	MaterialProperties material;
	blendMode blendMode;

	bool twoSided;
	bool hasChanged;

	std::vector<aiVector3D> animatedPos;
	std::vector<aiVector3D> animatedNorm;

	glm::mat4 matrix;
	glm::vec3 scaling;
	glm::vec3 translation;
};
}

#endif
