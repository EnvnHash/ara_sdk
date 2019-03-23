#ifndef __pebre_core__AssimpImport__
#define __pebre_core__AssimpImport__

#include <cassert>
#include <thread>
#include <unistd.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>

#include <glm/gtc/type_ptr.hpp>
#include "headers/gl_header.h"
#include "Shaders/ShaderUtils/MaterialProperties.h"

#include "AssimpUtils.h"
#include "AssimpMeshHelper.h"
#include "AssimpAnimation.h"

#include "GLUtils/GLMCamera.h"
#include "GLUtils/BoundingBoxer.h"
#include "Shaders/ShaderCollector.h"

namespace tav
{
class AssimpImport
{
public:
	enum drawMode {
		MESH_WIREFRAME, MESH_FILL, MESH_POINTS
	};

	typedef std::function<void(AssimpImport*)> uploadGLCb;

	AssimpImport(sceneData* _scd, bool _drawIndexed, bool _debug = false);
	~AssimpImport();

	void							load(const char* _filePath, SceneNode* _modelRoot, std::function<void()> _cb);
	virtual void					loadThread(bool optimize=true);

	void							createEmptyModel();
	void							createLightsFromAiModel();
	void							optimizeScene();
	std::string						GetDirectory(std::string& _path);

	void							update(double time);
	void							uploadToGL();
	void							normalizePositions(SceneNode* tNode, aiMesh* tMesh);

	void							initNormShader();
	void							deleteNormShader();

	bool							hasAnimations();
	unsigned int					getAnimationCount();
	AssimpAnimation&				getAnimation(int animationIndex);
	void							playAllAnimations();
	void							stopAllAnimations();
	void							resetAllAnimations();
	void							setPausedForAllAnimations(bool pause);
	void							setLoopStateForAllAnimations(enum AssimpAnimation::loopType state);
	void							setPositionForAllAnimations(float position);

	bool							hasMeshes();
	unsigned int					getMeshCount();
	AssimpMeshHelper*				getMeshHelper(int meshIndex);

	void							clear();

	std::vector<std::string>		getMeshNames();
	int								getNumMeshes();

	Mesh*							getMesh(std::string name);
	Mesh*							getMesh(int num);
	VAO*							getVao(int num);

	Mesh*							getCurrentAnimatedMesh(std::string name);
	Mesh*							getCurrentAnimatedMesh(int num);

	MaterialProperties*				getMaterialForMesh(std::string name);
	MaterialProperties*				getMaterialForMesh(int num);

	TextureManager*					getTextureForMesh(std::string name, int ind);
	TextureManager*					getTextureForMesh(int num, int ind);

	void							drawWireframe();
	void							drawFaces();
	void							drawVertices();

	void							enableTextures();
	void							disableTextures();
	void							enableNormals();
	void							enableMaterials();
	void							disableNormals();
	void							enableColors();
	void							disableColors();
	void							disableMaterials();

	void							drawMeshNoShdr(unsigned int meshNr, GLenum renderType);
	void							sendMeshMatToShdr(unsigned int meshNr, Shaders* shdr);

	void							drawMesh(unsigned int meshNr, GLenum renderType, Shaders* shdr);
	void							draw(GLenum renderType, Shaders* shdr = 0);
	void							drawInstanced(GLenum renderType, unsigned int nrInst,
													Shaders* shdr = 0);
	void							drawNoText(unsigned int meshNr, GLenum renderType, Shaders* shdr);
	void							drawNormCenter(GLenum renderType, Shaders* shdr = 0);

	glm::vec3						getPosition();
	glm::vec3						getSceneCenter();
	float							getNormalizedScale();
	glm::vec3						getScale();
	glm::mat4*						getNormAndCenterMatr();
	glm::mat4*						getNormCubeAndCenterMatr();
	glm::mat4*						getNormAndCenterZOffsMatr();

	glm::vec3						getDimensions();
	glm::vec3						getCenter();

	glm::vec3						getSceneMin(bool bScaled = false);
	glm::vec3						getSceneMax(bool bScaled = false);

	int								getNumRotations();	// returns the no. of applied rotations
	glm::vec3						getRotationAxis(int which); // gets each rotation axis
	float							getRotationAngle(int which); //gets each rotation angle

	const aiScene*					getAssimpScene();

	sceneData*						scd;
	SceneNode*						modelRoot;
	ShaderCollector*				shCol;

//	const aiScene*					scene;
	std::shared_ptr<const aiScene>	scene;
	std::shared_ptr<aiPropertyStore> store;

	// the main Asset Import scene that does the magic.
	Assimp::Importer*				import;

	std::string						filePath;
	std::string						fileAbsolutePath;

	bool							glOk;
	bool							loadSuccess;
	bool							useModelRoot;

	bool							normalizeScale;
	bool							drawIndexed;
	bool							bUsingTextures;
	bool							bUsingNormals;
	bool							bUsingColors;
	bool							bUsingMaterials;

protected:
	void							updateAnimations(double time);
	void							updateMeshes(aiNode * node, glm::mat4 parentMatrix);
	void							updateBones();

	// ai scene setup
	unsigned int					initImportProperties(bool optimize);

	// Updates the internal animation transforms for the selected animation index
	void							updateAnimation(unsigned int animationIndex, float time);

	// updates the *actual GL resources* for the current animation
	void							updateGLResources();

	void							get_bounding_box(aiVector3D* min, aiVector3D* max);
	void							get_bounding_box_for_node(const aiNode* nd, aiVector3D* min,
																aiVector3D* max, aiMatrix4x4* trafo);

	bool							debug;
	bool							debugGL;
	bool							isObj;

	float							normalizeFactor;
	double							normalizedScale;

	int								scrWidth;
	int								scrHeight;

	std::function<void()> 			loadEndCb;
	BoundingBoxer*					bBox;
	GLMCamera*						cam;

	std::vector<float>				rotAngle;
	std::vector<glm::vec3>			rotAxis;
	glm::vec3						scale;
	glm::vec3						pos;
	glm::mat4						modelMatrix;
	glm::mat4						normAndCenterMatr;
	glm::mat4						normAndCenterZOffsMatr;
	glm::mat4						normCubeAndCenterMatr;
	glm::mat3						normalMatr;

	//std::vector<Light> lights;
	//std::vector<AssimpTexture>	textures;
	std::vector<AssimpMeshHelper*>	modelMeshes;
	std::vector<AssimpAnimation>	animations;



	aiVector3D						scene_min, scene_max, scene_center;
	aiLogStream						stream;

	std::chrono::time_point<std::chrono::system_clock> startTime;
	std::chrono::time_point<std::chrono::system_clock> endTime;

	GLuint							normShader;
	GLuint							m_programPipeline;
};
}

#endif /* defined(__pebre_core__AssimpImport__) */
