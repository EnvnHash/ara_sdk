/*
 * FlexUtil.h
 *
 *  Created on: 20.07.2017
 *      Copyright by Sven Hahne
 */

#ifndef SRC_FLEX_FLEXUTIL_H_
#define SRC_FLEX_FLEXUTIL_H_

#include <map>

#include "headers/gl_header.h"
#include "GLUtils/glm_utils.h"
#include "GLUtils/FBO.h"
#include "Shaders/ShaderCollector.h"

#include <GLFW/glfw3.h>
#include "math_utils.h"

#include <cuda.h>
#include <cudaGL.h>
#include <cuda_gl_interop.h>

#include <flex/NvFlex.h>
#include <flex/NvFlexDevice.h>
#include <flex/NvFlexExt.h>
#include <flex/core/mesh.h>
#include <flex/core/sdf.h>
#include <flex/core/voxelize.h>

#include "flex/FlexStructs.h"

namespace tav
{

class FlexUtil
{
public:

	FlexUtil(ShaderCollector* _shCol);
	virtual ~FlexUtil();

	void init();
	void warmup();
	void update();
	void UpdateEmitters();
	void UpdateWind();
	void UpdateFluidRenderBuffers(FlexFluidRenderBuffers buffers, NvFlexSolver* solver, bool anisotropy, bool density);
	void UpdateFluidRenderBuffers(FlexFluidRenderBuffers buffers, glm::vec4* particles, float* densities,
			glm::vec4* anisotropy1, glm::vec4* anisotropy2, glm::vec4* anisotropy3, int numParticles, int* indices, int numIndices);
	void UpdateDiffuseRenderBuffers(FlexDiffuseRenderBuffers buffers, NvFlexSolver* solver);
	void UpdateDiffuseRenderBuffers(FlexDiffuseRenderBuffers buffers, glm::vec4* diffusePositions, glm::vec4* diffuseVelocities,
			int* diffuseIndices, int numDiffuseParticles);

	static void ErrorCallback(NvFlexErrorSeverity, const char* msg, const char* file, int line);

	void CreateParticleShape(const FlexMesh* srcMesh, glm::vec3 lower, glm::vec3 scale, float rotation, float spacing,
			glm::vec3 velocity, float invMass, bool rigid, float rigidStiffness, int phase, bool skin,
			float jitter=0.005f, glm::vec3 skinOffset=glm::vec3(0.0f), float skinExpand=0.0f, glm::vec4 color=glm::vec4(0.0f),
			float springStiffness=0.0f);
	void CreateParticleShape(const char* filename, glm::vec3 lower, glm::vec3 scale, float rotation, float spacing,
			glm::vec3 velocity, float invMass, bool rigid, float rigidStiffness, int phase, bool skin, float jitter=0.005f,
			glm::vec3 skinOffset=glm::vec3(0.0f), float skinExpand=0.0f, glm::vec4 color=glm::vec4(0.0f), float springStiffness=0.0f);
	void CreateParticleGrid(glm::vec3 lower, int dimx, int dimy, int dimz, float radius, glm::vec3 velocity,
			float invMass, bool rigid, float rigidStiffness, int phase, float jitter=0.005f);
	void CreateSpring(int i, int j, float stiffness, float give=0.0f);

	void CreateFluidRenderer(int width, int height, GLuint fbo);
	FlexFluidRenderBuffers CreateFluidRenderBuffers(int numFluidParticles, bool enableInterop);
	FlexDiffuseRenderBuffers CreateDiffuseRenderBuffers(int numDiffuseParticles, bool& enableInterop);

	Shaders* initPointThickShdr();
	Shaders* initEllipsoidDepthShdr();
	Shaders* initCompositeShdr();
	Shaders* initDepthBlurShdr();

	float SampleSDF(const float* sdf, int dim, int x, int y, int z);
	glm::vec3 SampleSDFGrad(const float* sdf, int dim, int x, int y, int z);

	void CalculateRigidLocalPositions(const glm::vec4* restPositions, int numRestPositions, const int* offsets,
			const int* indices, int numRigids, glm::vec3* localPositions);

	void GetParticleBounds(glm::vec3& lower, glm::vec3& upper);
	void GetShapeBounds(glm::vec3& totalLower, glm::vec3& totalUpper);

	void MapBuffers();
	void UnmapBuffers();

	void DestroyBuffers(FlexSimBuffers* buffers);
	void DestroyGpuMesh(FlexGpuMesh* m);
	void DestroyFluidRenderBuffers(FlexFluidRenderBuffers buffers);
	void DestroyDiffuseRenderBuffers(FlexDiffuseRenderBuffers buffers);
	void SkinMesh();

	void pushEmitter(FlexEmitter& e);

	void setDiffuseScale(float v);
	void setDrawPoints(bool v);
	void setDrawEllipsoids(bool v);
	void setDrawDiffuse(bool v);
	void setFluidColor(glm::vec4 col);
	void setLightDistance(float v);
	void setMaxDiffuseParticles(float v);
	void setNumExtraParticles(unsigned int v);
	void setNumSolidParticles(int val);
	void setNumSubsteps(unsigned int v);
	void setSceneLower(glm::vec3 v);
	void setWaveAmplitude(float v);
	void setWaveFloorTilt(float v);
	void setWaveFrequency(float v);

	double getDt();
	FlexFluidRenderer* getFluidRenderer();
	float getLightDistance();
	unsigned int getNumActPart();
	unsigned int getNumDiffPart();
	NvFlexParams* getParams();
	NvFlexVector<glm::vec4>* getSimBufPos();

	static bool 				flex_Error;

	glm::vec3 					g_camPos;
	glm::vec3 					g_camAngle;

	glm::vec3					g_sceneUpper;
	glm::vec3					g_sceneLower;

	float						g_lightDistance;
	float						g_pointScale = 1.0f;

	NvFlexParams 				g_params;
	std::vector<int> 			g_meshSkinIndices;

private:
	FBO*						depthFbo;
	FBO*						sceneFbo;
	FBO*						thickFbo;

	ShaderCollector* 			shCol;
	Shaders*					pointThickShdr;
	Shaders*					ellipsoidDepthShdr;
	Shaders*					compositeShdr;
	Shaders*					depthBlurShdr;


	NvFlexSolver* 				g_flex=0;
	NvFlexLibrary*				g_flexLib=0;

	FlexFluidRenderer*			g_fluidRenderer=0;
	FlexFluidRenderBuffers		g_fluidRenderBuffers;
	FlexDiffuseRenderBuffers	g_diffuseRenderBuffers;

	std::map<NvFlexConvexMeshId, FlexGpuMesh*> g_convexes;
	std::map<NvFlexTriangleMeshId, FlexGpuMesh*> g_meshes;
	std::map<NvFlexDistanceFieldId, FlexGpuMesh*> g_fields;

	std::vector<FlexRope> 		g_ropes;


	FlexMesh* 					g_mesh=0;
	FlexSimBuffers*				g_buffers=0;

	std::vector<FlexEmitter>	g_emitters;	// first emitter is the camera 'gun'
	std::vector<float> 			g_meshSkinWeights;
	std::vector<glm::vec3> 		g_meshRestPositions;

	const int					g_numSkinWeights = 4;

	int 						g_device = -1;
	int							g_frame = 0;
	int							g_maxDiffuseParticles;
	int							g_maxNeighborsPerParticle;
	int							g_mouseParticle;
	int							g_numExtraParticles;
	int							g_numExtraMultiplier=1;
	int							g_numSubsteps;
	int							g_numSolidParticles;

	float						g_blur;
	float						g_diffuseScale;
	float 						g_dt = 1.0f / 60.0f;	// the time delta used for simulation
	float						g_expandCloth;
	float						g_fogDistance = 0.005f;
	float						g_ior = 1.0f;
	float						g_waveAmplitude;
	float						g_waveFloorTilt;
	float 						g_waveFrequency = 1.5f;
	float						g_wavePlane;
	float						g_waveTime = 0.0f;
	float 						g_windFrequency = 0.1f;
	float						g_windTime = 0.0f;
	float						g_windStrength = 1.0f;

	float						g_camSpeed = 0.075f;
	float						g_camNear = 0.01f;
	float						g_camFar = 1000.0f;

	float						g_ropeScale = 1.0f;
	float						g_drawPlaneBias = 0.0f;
	float						g_diffuseMotionScale = 1.0f;
	float						g_diffuseInscatter = 0.8f;
	float						g_diffuseOutscatter = 0.53f;
	float						g_realdt;

	bool						g_diffuseShadow = false;
	bool						g_emit;
	bool 						g_extensions = true;
	bool						g_drawPoints;
	bool						g_drawEllipsoids;
	bool						g_drawCloth;
	bool						g_drawOpaque = false;
	bool						g_drawSprings = false;
	bool						g_drawDiffuse = false;
	bool						g_drawMesh = true;
	bool						g_drawRopes = true;
	bool						g_drawDensity = false;
	bool						g_interop = false;
	bool						g_pause = false;
	bool 						g_profile = false;
	bool						g_shapesChanged = false;
	bool 						g_step = false;
	bool						g_warmup;
	bool						g_wavePool=false;

	glm::vec4*					gColors=0;
	glm::vec4					g_diffuseColor;
	glm::vec4					g_fluidColor;

	glm::vec3					g_meshColor;

	glm::vec3 					g_camVel;
	glm::vec3 					g_camSmoothVel;

};

} /* namespace tav */

#endif /* SRC_FLEX_FLEXUTIL_H_ */
