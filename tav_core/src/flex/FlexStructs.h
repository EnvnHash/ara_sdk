/*
 * FlexStructs.h
 *
 *  Created on: 20.07.2017
 *      Copyright by Sven Hahne
 */

#ifndef SRC_FLEX_FLEXSTRUCTS_H_
#define SRC_FLEX_FLEXSTRUCTS_H_

namespace tav
{

struct FlexEmitter
{
	FlexEmitter() : mSpeed(0.0f), mEnabled(false), mLeftOver(0.0f), mWidth(8)   {}

	glm::vec3 mPos;
	glm::vec3 mDir;
	glm::vec3 mRight;
	float mSpeed;
	bool mEnabled;
	float mLeftOver;
	int mWidth;
};

class FlexSimBuffers
{
public:
	FlexSimBuffers(NvFlexLibrary* l) :
			positions(l), restPositions(l), velocities(l), phases(l), densities(l),
			anisotropy1(l), anisotropy2(l), anisotropy3(l), normals(l), smoothPositions(l),
			diffusePositions(l), diffuseVelocities(l), diffuseIndices(l), activeIndices(l),
			shapeGeometry(l), shapePositions(l), shapeRotations(l), shapePrevPositions(l),
			shapePrevRotations(l),	shapeFlags(l), rigidOffsets(l), rigidIndices(l), rigidMeshSize(l),
			rigidCoefficients(l), rigidRotations(l), rigidTranslations(l),
			rigidLocalPositions(l), rigidLocalNormals(l), inflatableTriOffsets(l),
			inflatableTriCounts(l), inflatableVolumes(l), inflatableCoefficients(l),
			inflatablePressures(l), springIndices(l), springLengths(l),
			springStiffness(l), triangles(l), triangleNormals(l), uvs(l)
	{}
	~FlexSimBuffers() {}

	NvFlexVector<glm::vec4> positions;
	NvFlexVector<glm::vec4> restPositions;
	NvFlexVector<glm::vec3> velocities;
	NvFlexVector<int> phases;
	NvFlexVector<float> densities;
	NvFlexVector<glm::vec4> anisotropy1;
	NvFlexVector<glm::vec4> anisotropy2;
	NvFlexVector<glm::vec4> anisotropy3;
	NvFlexVector<glm::vec4> normals;
	NvFlexVector<glm::vec4> smoothPositions;
	NvFlexVector<glm::vec4> diffusePositions;
	NvFlexVector<glm::vec4> diffuseVelocities;
	NvFlexVector<int> diffuseIndices;
	NvFlexVector<int> activeIndices;

	// convexes
	NvFlexVector<NvFlexCollisionGeometry> shapeGeometry;
	NvFlexVector<glm::vec4> shapePositions;
	NvFlexVector<glm::quat> shapeRotations;
	NvFlexVector<glm::vec4> shapePrevPositions;
	NvFlexVector<glm::quat> shapePrevRotations;
	NvFlexVector<int> shapeFlags;

	// rigids
	NvFlexVector<int> rigidOffsets;
	NvFlexVector<int> rigidIndices;
	NvFlexVector<int> rigidMeshSize;
	NvFlexVector<float> rigidCoefficients;
	NvFlexVector<glm::quat> rigidRotations;
	NvFlexVector<glm::vec3> rigidTranslations;
	NvFlexVector<glm::vec3> rigidLocalPositions;
	NvFlexVector<glm::vec4> rigidLocalNormals;

	// inflatables
	NvFlexVector<int> inflatableTriOffsets;
	NvFlexVector<int> inflatableTriCounts;
	NvFlexVector<float> inflatableVolumes;
	NvFlexVector<float> inflatableCoefficients;
	NvFlexVector<float> inflatablePressures;

	// springs
	NvFlexVector<int> springIndices;
	NvFlexVector<float> springLengths;
	NvFlexVector<float> springStiffness;

	NvFlexVector<int> triangles;
	NvFlexVector<glm::vec3> triangleNormals;
	NvFlexVector<glm::vec3> uvs;
};

struct FlexFluidRenderer
{
	GLuint mDepthFbo;
	GLuint mDepthTex;
	GLuint mDepthSmoothTex;
	GLuint mSceneFbo=0;
	GLuint mSceneTex;
	GLuint mReflectTex;

	GLuint mThicknessFbo;
	GLuint mThicknessTex;

	GLuint mPointThicknessProgram;
	//GLuint mPointDepthProgram;

	GLuint mEllipsoidThicknessProgram;
	GLuint mEllipsoidDepthProgram;

	GLuint mCompositeProgram;
	GLuint mDepthBlurProgram;

	int mSceneWidth;
	int mSceneHeight;
};

struct FlexFluidRenderBuffers
{
	GLuint mPositionVBO;
	GLuint mDensityVBO;
	GLuint mAnisotropyVBO[3];
	GLuint mIndices;

	GLuint mFluidVBO; // to be removed

	// wrapper buffers that allow Flex to write directly to VBOs
	NvFlexBuffer* mPositionBuf;
	NvFlexBuffer* mDensitiesBuf;
	NvFlexBuffer* mAnisotropyBuf[3];
	NvFlexBuffer* mIndicesBuf;

	int mNumFluidParticles;
};

// vertex buffers for diffuse particles
typedef struct
{
	GLuint mDiffusePositionVBO;
	GLuint mDiffuseVelocityVBO;
	GLuint mDiffuseIndicesIBO;

	NvFlexBuffer* mDiffuseIndicesBuf;
	NvFlexBuffer* mDiffusePositionsBuf;
	NvFlexBuffer* mDiffuseVelocitiesBuf;

	int mNumDiffuseParticles;
} FlexDiffuseRenderBuffers;

struct FlexGpuMesh
{
	GLuint mPositionsVBO;
	GLuint mNormalsVBO;
	GLuint mIndicesIBO;

	int mNumVertices;
	int mNumFaces;
};

struct FlexRope
{
	std::vector<int> mIndices;
};


}


#endif /* SRC_FLEX_FLEXSTRUCTS_H_ */
