/*
 * FlexUtil.cpp
 *
 *  Created on: 20.07.2017
 *      Copyright by Sven Hahne
 */

#include "pch.h"
#include "FlexUtil.h"

#define STRINGIFY(A) #A

namespace tav
{

bool FlexUtil::flex_Error;

FlexUtil::FlexUtil(ShaderCollector* _shCol) : shCol(_shCol)
{
	init();

	g_emitters = std::vector<FlexEmitter>(1);

	if (g_flex)
	{
		if (g_buffers)
			DestroyBuffers(g_buffers);

		DestroyFluidRenderBuffers(g_fluidRenderBuffers);
		DestroyDiffuseRenderBuffers(g_diffuseRenderBuffers);

		for (auto& iter : g_meshes)
		{
			NvFlexDestroyTriangleMesh(g_flexLib, iter.first);
			DestroyGpuMesh(iter.second);
		}

		for (auto& iter : g_fields)
		{
			NvFlexDestroyDistanceField(g_flexLib, iter.first);
			DestroyGpuMesh(iter.second);
		}

		for (auto& iter : g_convexes)
		{
			NvFlexDestroyConvexMesh(g_flexLib, iter.first);
			DestroyGpuMesh(iter.second);
		}


		g_fields.clear();
		g_meshes.clear();
		g_convexes.clear();

		NvFlexDestroySolver(g_flex);
		g_flex = NULL;
	}

	// alloc buffers
	g_buffers = new FlexSimBuffers(g_flexLib);

	// map during initialization
	MapBuffers();

	g_buffers->positions.resize(0);
	g_buffers->velocities.resize(0);
	g_buffers->phases.resize(0);

	g_buffers->rigidOffsets.resize(0);
	g_buffers->rigidIndices.resize(0);
	g_buffers->rigidMeshSize.resize(0);
	g_buffers->rigidRotations.resize(0);
	g_buffers->rigidTranslations.resize(0);
	g_buffers->rigidCoefficients.resize(0);
	g_buffers->rigidLocalPositions.resize(0);
	g_buffers->rigidLocalNormals.resize(0);

	g_buffers->springIndices.resize(0);
	g_buffers->springLengths.resize(0);
	g_buffers->springStiffness.resize(0);
	g_buffers->triangles.resize(0);
	g_buffers->triangleNormals.resize(0);
	g_buffers->uvs.resize(0);

	g_meshSkinIndices.resize(0);
	g_meshSkinWeights.resize(0);

	g_emitters.resize(1);
	g_emitters[0].mEnabled = false;
	g_emitters[0].mSpeed = 1.0f;

	g_buffers->shapeGeometry.resize(0);
	g_buffers->shapePositions.resize(0);
	g_buffers->shapeRotations.resize(0);
	g_buffers->shapePrevPositions.resize(0);
	g_buffers->shapePrevRotations.resize(0);
	g_buffers->shapeFlags.resize(0);

	g_ropes.resize(0);

	// remove collision shapes
	delete g_mesh;
	g_mesh = 0;

	g_frame = 0;
	g_pause = false;

	g_dt = 1.0f / 60.0f;
	g_waveTime = 0.0f;
	g_windTime = 0.0f;
	g_windStrength = 1.0f;

	g_blur = 1.0f;
	g_fluidColor = glm::vec4(0.1f, 0.4f, 0.8f, 1.0f);
	g_meshColor = glm::vec3(0.9f, 0.9f, 0.9f);
	g_drawEllipsoids = false;
	g_drawPoints = true;
	g_drawCloth = true;
	g_expandCloth = 0.0f;

	g_drawOpaque = false;
	g_drawSprings = false;
	g_drawDiffuse = false;
	g_drawMesh = true;
	g_drawRopes = true;
	g_drawDensity = false;
	g_ior = 1.0f;
	g_lightDistance = 2.0f;
	g_fogDistance = 0.005f;

	g_camSpeed = 0.075f;
	g_camNear = 0.01f;
	g_camFar = 1000.0f;

	g_pointScale = 1.0f;
	g_ropeScale = 1.0f;
	g_drawPlaneBias = 0.0f;

	// sim params
	g_params.gravity[0] = 0.0f;
	g_params.gravity[1] = -9.8f;
	g_params.gravity[2] = 0.0f;

	g_params.wind[0] = 0.0f;
	g_params.wind[1] = 0.0f;
	g_params.wind[2] = 0.0f;

	g_params.radius = 0.15f;
	g_params.viscosity = 0.0f;
	g_params.dynamicFriction = 0.0f;
	g_params.staticFriction = 0.0f;
	g_params.particleFriction = 0.0f; // scale friction between particles by default
	g_params.freeSurfaceDrag = 0.0f;
	g_params.drag = 0.0f;
	g_params.lift = 0.0f;
	g_params.numIterations = 3;
	g_params.fluidRestDistance = 0.0f;
	g_params.solidRestDistance = 0.0f;

	g_params.anisotropyScale = 1.0f;
	g_params.anisotropyMin = 0.1f;
	g_params.anisotropyMax = 2.0f;
	g_params.smoothing = 1.0f;

	g_params.dissipation = 0.0f;
	g_params.damping = 0.0f;
	g_params.particleCollisionMargin = 0.0f;
	g_params.shapeCollisionMargin = 0.0f;
	g_params.collisionDistance = 0.0f;
	g_params.plasticThreshold = 0.0f;
	g_params.plasticCreep = 0.0f;
	g_params.fluid = false;
	g_params.sleepThreshold = 0.0f;
	g_params.shockPropagation = 0.0f;
	g_params.restitution = 0.0f;

	g_params.maxSpeed = FLT_MAX;
	g_params.maxAcceleration = 100.0f;	// approximately 10x gravity

	g_params.relaxationMode = eNvFlexRelaxationLocal;
	g_params.relaxationFactor = 1.0f;
	g_params.solidPressure = 1.0f;
	g_params.adhesion = 0.0f;
	g_params.cohesion = 0.025f;
	g_params.surfaceTension = 0.0f;
	g_params.vorticityConfinement = 0.0f;
	g_params.buoyancy = 1.0f;
	g_params.diffuseThreshold = 100.0f;
	g_params.diffuseBuoyancy = 1.0f;
	g_params.diffuseDrag = 0.8f;
	g_params.diffuseBallistic = 16;
	g_params.diffuseSortAxis[0] = 0.0f;
	g_params.diffuseSortAxis[1] = 0.0f;
	g_params.diffuseSortAxis[2] = 0.0f;
	g_params.diffuseLifetime = 2.0f;

	g_numSubsteps = 2;

	// planes created after particles
	g_params.numPlanes = 1;

	g_diffuseScale = 0.5f;
	g_diffuseColor = glm::vec4(1.0f);
	g_diffuseMotionScale = 1.0f;
	g_diffuseShadow = false;
	g_diffuseInscatter = 0.8f;
	g_diffuseOutscatter = 0.53f;

	// reset phase 0 particle color to blue
	gColors = new glm::vec4[1];
	gColors[0] = glm::vec4(0.0f, 0.5f, 1.0f, 1.f);

	g_numSolidParticles = 0;

	g_waveFrequency = 1.5f;
	g_waveAmplitude = 1.5f;
	g_waveFloorTilt = 0.0f;
	g_emit = false;
	g_warmup = false;

	g_mouseParticle = -1;

	g_maxDiffuseParticles = 0;	// number of diffuse particles
	g_maxNeighborsPerParticle = 96;
	g_numExtraParticles = 0;	// number of particles allocated but not made active

	g_sceneLower = glm::vec3(FLT_MAX);
	g_sceneUpper = glm::vec3(-FLT_MAX);

	// create scene
	//g_scenes[g_scene]->Initialize();
}

//---------------------------------------------------------------

void FlexUtil::warmup()
{
	uint32_t numParticles = g_buffers->positions.size();
	uint32_t maxParticles = numParticles + g_numExtraParticles * g_numExtraMultiplier;

	// by default solid particles use the maximum radius
	if (g_params.fluid && g_params.solidRestDistance == 0.0f)
		g_params.solidRestDistance = g_params.fluidRestDistance;
	else
		g_params.solidRestDistance = g_params.radius;

	// collision distance with shapes half the radius
	if (g_params.collisionDistance == 0.0f)
	{
		g_params.collisionDistance = g_params.radius*0.5f;

		if (g_params.fluid)
			g_params.collisionDistance = g_params.fluidRestDistance*0.5f;
	}

	// default particle friction to 10% of shape friction
	if (g_params.particleFriction == 0.0f)
		g_params.particleFriction = g_params.dynamicFriction*0.1f;

	// add a margin for detecting contacts between particles and shapes
	if (g_params.shapeCollisionMargin == 0.0f)
		g_params.shapeCollisionMargin = g_params.collisionDistance*0.5f;

	// calculate particle bounds
	glm::vec3 particleLower, particleUpper;
	GetParticleBounds(particleLower, particleUpper);

	// accommodate shapes
	glm::vec3 shapeLower, shapeUpper;
	GetShapeBounds(shapeLower, shapeUpper);

	// update bounds
	g_sceneLower = glm::min(glm::min(g_sceneLower, particleLower), shapeLower);
	g_sceneUpper = glm::max(glm::max(g_sceneUpper, particleUpper), shapeUpper);

	g_sceneLower -= g_params.collisionDistance;
	g_sceneUpper += g_params.collisionDistance;

	// update collision planes to match flexs
	glm::vec3 up = Normalize(glm::vec3(-g_waveFloorTilt, 1.0f, 0.0f));

	(glm::vec4&)g_params.planes[0] = glm::vec4(up.x, up.y, up.z, 0.0f);
	(glm::vec4&)g_params.planes[1] = glm::vec4(0.0f, 0.0f, 1.0f, -g_sceneLower.z);
	(glm::vec4&)g_params.planes[2] = glm::vec4(1.0f, 0.0f, 0.0f, -g_sceneLower.x);
	(glm::vec4&)g_params.planes[3] = glm::vec4(-1.0f, 0.0f, 0.0f, g_sceneUpper.x);
	(glm::vec4&)g_params.planes[4] = glm::vec4(0.0f, 0.0f, -1.0f, g_sceneUpper.z);
	(glm::vec4&)g_params.planes[5] = glm::vec4(0.0f, -1.0f, 0.0f, g_sceneUpper.y);

	g_wavePlane = g_params.planes[2][3];

	g_buffers->diffusePositions.resize(g_maxDiffuseParticles);
	g_buffers->diffuseVelocities.resize(g_maxDiffuseParticles);
	g_buffers->diffuseIndices.resize(g_maxDiffuseParticles);

	// for fluid rendering these are the Laplacian smoothed positions
	g_buffers->smoothPositions.resize(maxParticles);

	g_buffers->normals.resize(0);
	g_buffers->normals.resize(maxParticles);

	// initialize normals (just for rendering before simulation starts)
	int numTris = g_buffers->triangles.size() / 3;
	for (int i = 0; i < numTris; ++i)
	{
		glm::vec3 v0 = glm::vec3(g_buffers->positions[g_buffers->triangles[i * 3 + 0]]);
		glm::vec3 v1 = glm::vec3(g_buffers->positions[g_buffers->triangles[i * 3 + 1]]);
		glm::vec3 v2 = glm::vec3(g_buffers->positions[g_buffers->triangles[i * 3 + 2]]);

		glm::vec3 n = glm::cross(v1 - v0, v2 - v0);

		g_buffers->normals[g_buffers->triangles[i * 3 + 0]] += glm::vec4(n, 0.0f);
		g_buffers->normals[g_buffers->triangles[i * 3 + 1]] += glm::vec4(n, 0.0f);
		g_buffers->normals[g_buffers->triangles[i * 3 + 2]] += glm::vec4(n, 0.0f);
	}

	for (int i = 0; i < int(maxParticles); ++i)
		g_buffers->normals[i] = glm::vec4(Normalize(glm::vec3(g_buffers->normals[i]), glm::vec3(0.0f, 1.0f, 0.0f)), 0.0f);


	// save mesh positions for skinning
	if (g_mesh)
	{
		g_meshRestPositions = g_mesh->m_positions;
	}
	else
	{
		g_meshRestPositions.resize(0);
	}

	// main create method for the Flex solver
	g_flex = NvFlexCreateSolver(g_flexLib, maxParticles, g_maxDiffuseParticles, g_maxNeighborsPerParticle);

	// give scene a chance to do some post solver initialization
	//g_scenes[g_scene]->PostInitialize();

	// center camera on particles
//	if (centerCamera)
//	{
		g_camPos = glm::vec3((g_sceneLower.x + g_sceneUpper.x) *0.5f,
				glm::min(g_sceneUpper.y*1.25f, 6.0f),
				g_sceneUpper.z + glm::min(g_sceneUpper.y, 6.0f)*2.0f);
		g_camAngle = glm::vec3(0.0f, -DegToRad(15.0f), 0.0f);

		// give scene a chance to modify camera position
	//	g_scenes[g_scene]->CenterCamera();
//	}

	// create active indices (just a contiguous block for the demo)
	g_buffers->activeIndices.resize(g_buffers->positions.size());
	for (int i = 0; i < g_buffers->activeIndices.size(); ++i)
		g_buffers->activeIndices[i] = i;

	// resize particle buffers to fit
	g_buffers->positions.resize(maxParticles);
	std::cout << "resize g_buffers->positions.buffer: " << g_buffers->positions.buffer << std::endl;



	g_buffers->velocities.resize(maxParticles);
	g_buffers->phases.resize(maxParticles);

	g_buffers->densities.resize(maxParticles);
	g_buffers->anisotropy1.resize(maxParticles);
	g_buffers->anisotropy2.resize(maxParticles);
	g_buffers->anisotropy3.resize(maxParticles);

	// save rest positions
	g_buffers->restPositions.resize(g_buffers->positions.size());
	for (int i = 0; i < g_buffers->positions.size(); ++i)
		g_buffers->restPositions[i] = g_buffers->positions[i];

	// builds rigids constraints
	if (g_buffers->rigidOffsets.size())
	{
		assert(g_buffers->rigidOffsets.size() > 1);

		const int numRigids = g_buffers->rigidOffsets.size() - 1;

		// calculate local rest space positions
		g_buffers->rigidLocalPositions.resize(g_buffers->rigidOffsets.back());
		CalculateRigidLocalPositions(&g_buffers->positions[0], g_buffers->positions.size(), &g_buffers->rigidOffsets[0], &g_buffers->rigidIndices[0], numRigids, &g_buffers->rigidLocalPositions[0]);

		g_buffers->rigidRotations.resize(g_buffers->rigidOffsets.size() - 1, glm::quat());
		g_buffers->rigidTranslations.resize(g_buffers->rigidOffsets.size() - 1, glm::vec3());

	}

	// unmap so we can start transferring data to GPU
	UnmapBuffers();

	//-----------------------------
	// Send data to Flex

	NvFlexSetParams(g_flex, &g_params);
	NvFlexSetParticles(g_flex, g_buffers->positions.buffer, numParticles);
	NvFlexSetVelocities(g_flex, g_buffers->velocities.buffer, numParticles);
	NvFlexSetNormals(g_flex, g_buffers->normals.buffer, numParticles);
	NvFlexSetPhases(g_flex, g_buffers->phases.buffer, g_buffers->phases.size());
	NvFlexSetRestParticles(g_flex, g_buffers->restPositions.buffer, g_buffers->restPositions.size());

	NvFlexSetActive(g_flex, g_buffers->activeIndices.buffer, numParticles);

	// springs
	if (g_buffers->springIndices.size())
	{
		assert((g_buffers->springIndices.size() & 1) == 0);
		assert((g_buffers->springIndices.size() / 2) == g_buffers->springLengths.size());

		NvFlexSetSprings(g_flex, g_buffers->springIndices.buffer, g_buffers->springLengths.buffer, g_buffers->springStiffness.buffer, g_buffers->springLengths.size());
	}

	// rigids
	if (g_buffers->rigidOffsets.size())
	{
		NvFlexSetRigids(g_flex, g_buffers->rigidOffsets.buffer, g_buffers->rigidIndices.buffer, g_buffers->rigidLocalPositions.buffer, g_buffers->rigidLocalNormals.buffer, g_buffers->rigidCoefficients.buffer, g_buffers->rigidRotations.buffer, g_buffers->rigidTranslations.buffer, g_buffers->rigidOffsets.size() - 1, g_buffers->rigidIndices.size());
	}

	// inflatables
	if (g_buffers->inflatableTriOffsets.size())
	{
		NvFlexSetInflatables(g_flex, g_buffers->inflatableTriOffsets.buffer, g_buffers->inflatableTriCounts.buffer, g_buffers->inflatableVolumes.buffer, g_buffers->inflatablePressures.buffer, g_buffers->inflatableCoefficients.buffer, g_buffers->inflatableTriOffsets.size());
	}

	// dynamic triangles
	if (g_buffers->triangles.size())
	{
		NvFlexSetDynamicTriangles(g_flex, g_buffers->triangles.buffer, g_buffers->triangleNormals.buffer, g_buffers->triangles.size() / 3);
	}

	// collision shapes
	if (g_buffers->shapeFlags.size())
	{
		NvFlexSetShapes(
				g_flex,
				g_buffers->shapeGeometry.buffer,
				g_buffers->shapePositions.buffer,
				g_buffers->shapeRotations.buffer,
				g_buffers->shapePrevPositions.buffer,
				g_buffers->shapePrevRotations.buffer,
				g_buffers->shapeFlags.buffer,
				int(g_buffers->shapeFlags.size()));
	}

	// create render buffers
	g_fluidRenderBuffers = CreateFluidRenderBuffers(maxParticles, g_interop);
	g_diffuseRenderBuffers = CreateDiffuseRenderBuffers(g_maxDiffuseParticles, g_interop);

	// perform initial sim warm up
	if (g_warmup)
	{
		printf("Warming up sim..\n");

		// warm it up (relax positions to reach rest density without affecting velocity)
		NvFlexParams copy = g_params;
		copy.numIterations = 4;

		NvFlexSetParams(g_flex, &copy);

		const int kWarmupIterations = 100;

		for (int i = 0; i < kWarmupIterations; ++i)
		{
			NvFlexUpdateSolver(g_flex, 0.0001f, 1, false);
			NvFlexSetVelocities(g_flex, g_buffers->velocities.buffer, maxParticles);
		}

		// udpate host copy
		NvFlexGetParticles(g_flex, g_buffers->positions.buffer, g_buffers->positions.size());
		NvFlexGetSmoothParticles(g_flex, g_buffers->smoothPositions.buffer, g_buffers->smoothPositions.size());
		NvFlexGetAnisotropy(g_flex, g_buffers->anisotropy1.buffer, g_buffers->anisotropy2.buffer, g_buffers->anisotropy3.buffer);

		printf("Finished warm up.\n");
	}
}

//---------------------------------------------------------------

void FlexUtil::init()
{
	// use the PhysX GPU selected from the NVIDIA control panel
	if (g_device == -1)
		g_device = NvFlexDeviceGetSuggestedOrdinal();

	// Init Cuda context
	unsigned int Flags=0;
	CUresult res = cuInit(Flags);
	if (res != CUDA_SUCCESS)
		std::cerr << " CUDA cuInit has failed: " << std::endl;
	else
		std::cout << "CUDA inited successfully " << std::endl;

	// Create an optimized CUDA context for Flex and set it on the
	// calling thread. This is an optional call, it is fine to use
	// a regular CUDA context, although creating one through this API
	// is recommended for best performance.
	if (!NvFlexDeviceCreateCudaContext(g_device))
	{
		std::cerr << "Error creating CUDA context." << std::endl;
		exit(-1);
	} else {
		std::cout << "NvFlexDeviceCreateCudaContext SUCCESS" << std::endl;
	}

	NvFlexInitDesc desc;
	desc.deviceIndex = g_device;
	desc.enableExtensions = g_extensions;
	desc.renderDevice = 0;
	desc.renderContext = 0;
	desc.computeType = eNvFlexCUDA;

	// Init Flex library, note that no CUDA methods should be called before this
	// point to ensure we get the device context we want
	g_flexLib = NvFlexInit(NV_FLEX_VERSION, &FlexUtil::ErrorCallback, &desc);

	if (flex_Error || g_flexLib == NULL)
	{
		printf("Could not initialize Flex, exiting.\n");
		exit(-1);
	} else
		printf("NvFlexInit success.\n");

	// store device name
	char g_deviceName[256];
	strcpy(g_deviceName, NvFlexGetDeviceName(g_flexLib));
	printf("Compute Device: %s\n\n", g_deviceName);
}

//---------------------------------------------------------------

void FlexUtil::update()
{
	static double lastTime;

	// real elapsed frame time
	double frameBeginTime = glfwGetTime();

	g_realdt = float(frameBeginTime - lastTime);
	lastTime = frameBeginTime;

	// do gamepad input polling
	double currentTime = frameBeginTime;

	if (!g_pause || g_step)
	{
		UpdateEmitters();
		UpdateWind();
		//UpdateMouse();
		//UpdateScene();
	}


	double updateBeginTime = glfwGetTime();

	// send any particle updates to the solver
	NvFlexSetParticles(g_flex, g_buffers->positions.buffer, g_buffers->positions.size());
	NvFlexSetVelocities(g_flex, g_buffers->velocities.buffer, g_buffers->velocities.size());
	NvFlexSetPhases(g_flex, g_buffers->phases.buffer, g_buffers->phases.size());
	NvFlexSetActive(g_flex, g_buffers->activeIndices.buffer, g_buffers->activeIndices.size());

	if (g_shapesChanged)
	{
		NvFlexSetShapes(
			g_flex,
			g_buffers->shapeGeometry.buffer,
			g_buffers->shapePositions.buffer,
			g_buffers->shapeRotations.buffer,
			g_buffers->shapePrevPositions.buffer,
			g_buffers->shapePrevRotations.buffer,
			g_buffers->shapeFlags.buffer,
			int(g_buffers->shapeFlags.size()));

		g_shapesChanged = false;
	}

	if (!g_pause || g_step)
	{
		// tick solver
		NvFlexSetParams(g_flex, &g_params);
		NvFlexUpdateSolver(g_flex, g_dt, g_numSubsteps, g_profile);

		g_frame++;
		g_step = false;
	}


	// read back base particle data
	// Note that flexGet calls don't wait for the GPU, they just queue a GPU copy
	// to be executed later.
	// When we're ready to read the fetched buffers we'll Map them, and that's when
	// the CPU will wait for the GPU flex update and GPU copy to finish.
	NvFlexGetParticles(g_flex, g_buffers->positions.buffer, g_buffers->positions.size());
	NvFlexGetVelocities(g_flex, g_buffers->velocities.buffer, g_buffers->velocities.size());
	NvFlexGetNormals(g_flex, g_buffers->normals.buffer, g_buffers->normals.size());

	// readback triangle normals
	if (g_buffers->triangles.size())
		NvFlexGetDynamicTriangles(g_flex, g_buffers->triangles.buffer, g_buffers->triangleNormals.buffer, g_buffers->triangles.size() / 3);

	// readback rigid transforms
	if (g_buffers->rigidOffsets.size())
		NvFlexGetRigidTransforms(g_flex, g_buffers->rigidRotations.buffer, g_buffers->rigidTranslations.buffer);

	if (!g_interop)
	{
		// if not using interop then we read back fluid data to host
		if (g_drawEllipsoids)
		{
			NvFlexGetSmoothParticles(g_flex, g_buffers->smoothPositions.buffer, g_buffers->smoothPositions.size());
			NvFlexGetAnisotropy(g_flex, g_buffers->anisotropy1.buffer, g_buffers->anisotropy2.buffer, g_buffers->anisotropy3.buffer);
		}

		// read back diffuse data to host
		if (g_drawDensity)
			NvFlexGetDensities(g_flex, g_buffers->densities.buffer, g_buffers->positions.size());

		if (g_diffuseRenderBuffers.mNumDiffuseParticles)
		{
			NvFlexGetDiffuseParticles(g_flex, g_buffers->diffusePositions.buffer, g_buffers->diffuseVelocities.buffer, g_buffers->diffuseIndices.buffer);
		}
	}

	double updateEndTime = glfwGetTime();

	//-------------------------------------------------------
	// Update the on-screen timers
/*
	float newUpdateTime = float(updateEndTime - updateBeginTime);
	float newRenderTime = float(renderEndTime - renderBeginTime);
	float newWaitTime = float(waitBeginTime - waitEndTime);

	// Exponential filter to make the display easier to read
	const float timerSmoothing = 0.05f;

	g_updateTime = (g_updateTime == 0.0f) ? newUpdateTime : glm::lerp(g_updateTime, newUpdateTime, timerSmoothing);
	g_renderTime = (g_renderTime == 0.0f) ? newRenderTime : glm::lerp(g_renderTime, newRenderTime, timerSmoothing);
	g_waitTime = (g_waitTime == 0.0f) ? newWaitTime : glm::lerp(g_waitTime, newWaitTime, timerSmoothing);
	g_simLatency = (g_simLatency == 0.0f) ? newSimLatency : glm::lerp(g_simLatency, newSimLatency, timerSmoothing);
	*/

	//---------------------------------------------------
	// Von RenderScene()

	const int numParticles = getNumActPart();
	const int numDiffuse = getNumDiffPart();

	//---------------------------------------------------
	// use VBO buffer wrappers to allow Flex to write directly to the OpenGL buffers
	// Flex will take care of any CUDA interop mapping/unmapping during the get() operations
	if (numParticles)
	{

		if (g_interop)
		{
			// copy data directly from solver to the renderer buffers
			UpdateFluidRenderBuffers(g_fluidRenderBuffers, g_flex, g_drawEllipsoids, g_drawDensity);
		}
		else
		{
			// copy particle data to GPU render device

			if (g_drawEllipsoids)
			{
				// if fluid surface rendering then update with smooth positions and anisotropy
				UpdateFluidRenderBuffers(g_fluidRenderBuffers,
					&g_buffers->smoothPositions[0],
					(g_drawDensity) ? &g_buffers->densities[0] : (float*)&g_buffers->phases[0],
					&g_buffers->anisotropy1[0],
					&g_buffers->anisotropy2[0],
					&g_buffers->anisotropy3[0],
					g_buffers->positions.size(),
					&g_buffers->activeIndices[0],
					numParticles);
			}
			else
			{
				// otherwise just send regular positions and no anisotropy
				UpdateFluidRenderBuffers(g_fluidRenderBuffers,
					&g_buffers->positions[0],
					(float*)&g_buffers->phases[0],
					NULL, NULL, NULL,
					g_buffers->positions.size(),
					&g_buffers->activeIndices[0],
					numParticles);
			}
		}
	}

	if (numDiffuse)
	{
		if (g_interop)
		{
			// copy data directly from solver to the renderer buffers
			UpdateDiffuseRenderBuffers(g_diffuseRenderBuffers, g_flex);
		}
		else
		{
			// copy diffuse particle data from host to GPU render device
			UpdateDiffuseRenderBuffers(g_diffuseRenderBuffers,
				&g_buffers->diffusePositions[0],
				&g_buffers->diffuseVelocities[0],
				&g_buffers->diffuseIndices[0],
				numDiffuse);
		}
	}
}

//---------------------------------------------------------------

void FlexUtil::UpdateEmitters()
{
	float spin = DegToRad(15.0f);

	const glm::vec3 forward(-sinf(g_camAngle.x + spin)*cosf(g_camAngle.y), sinf(g_camAngle.y), -cosf(g_camAngle.x + spin)*cosf(g_camAngle.y));
	const glm::vec3 right(Normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f))));

	g_emitters[0].mDir = Normalize(forward + glm::vec3(0.0, 0.4f, 0.0f));
	g_emitters[0].mRight = right;
	g_emitters[0].mPos = g_camPos + forward*1.f + glm::vec3(0.0f, 0.2f, 0.0f) + right*0.65f;

	// process emitters
	if (g_emit)
	{
		int activeCount = NvFlexGetActiveCount(g_flex);

		size_t e = 0;

		// skip camera emitter when moving forward or things get messy
		if (g_camSmoothVel.z >= 0.025f)
			e = 1;

		for (; e < g_emitters.size(); ++e)
		{
			if (!g_emitters[e].mEnabled)
				continue;

			glm::vec3 emitterDir = g_emitters[e].mDir;
			glm::vec3 emitterRight = g_emitters[e].mRight;
			glm::vec3 emitterPos = g_emitters[e].mPos;

			float r;
			int phase;

			if (g_params.fluid)
			{
				r = g_params.fluidRestDistance;
				phase = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid);
			}
			else
			{
				r = g_params.solidRestDistance;
				phase = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide);
			}

			float numParticles = (g_emitters[e].mSpeed / r)*g_dt;

			// whole number to emit
			int n = int(numParticles + g_emitters[e].mLeftOver);

			if (n)
				g_emitters[e].mLeftOver = (numParticles + g_emitters[e].mLeftOver) - n;
			else
				g_emitters[e].mLeftOver += numParticles;

			// create a grid of particles (n particles thick)
			for (int k = 0; k < n; ++k)
			{
				int emitterWidth = g_emitters[e].mWidth;
				int numParticles = emitterWidth*emitterWidth;
				for (int i = 0; i < numParticles; ++i)
				{
					float x = float(i%emitterWidth) - float(emitterWidth/2);
					float y = float((i / emitterWidth) % emitterWidth) - float(emitterWidth/2);

					if ((std::sqrt(x) + std::sqrt(y)) <= (emitterWidth / 2)*(emitterWidth / 2))
					{
						glm::vec3 up = Normalize(glm::cross(emitterDir, emitterRight));
						glm::vec3 offset = r*(emitterRight*x + up*y) + float(k)*emitterDir*r;

						if (activeCount < g_buffers->positions.size())
						{
							g_buffers->positions[activeCount] = glm::vec4(emitterPos + offset, 1.0f);
							g_buffers->velocities[activeCount] = emitterDir*g_emitters[e].mSpeed;
							g_buffers->phases[activeCount] = phase;

							g_buffers->activeIndices.push_back(activeCount);

							activeCount++;
						}
					}
				}
			}
		}
	}
}

//---------------------------------------------------------------

void FlexUtil::UpdateWind()
{
	g_windTime += g_dt;

	const glm::vec3 kWindDir = glm::vec3(3.0f, 15.0f, 0.0f);
	const float kNoise = perlinOct1D(g_windTime * g_windFrequency, 10, 0.25f);
	glm::vec3 wind = g_windStrength * kWindDir * glm::vec3(kNoise, fabsf(kNoise), 0.0f);

	g_params.wind[0] = wind.x;
	g_params.wind[1] = wind.y;
	g_params.wind[2] = wind.z;

	if (g_wavePool)
	{
		g_waveTime += g_dt;

		g_params.planes[2][3] = g_wavePlane + (sinf(float(g_waveTime)*g_waveFrequency - kPi*0.5f)*0.5f + 0.5f)*g_waveAmplitude;
	}
}

//---------------------------------------------------------------

void FlexUtil::UpdateFluidRenderBuffers(FlexFluidRenderBuffers buffers, NvFlexSolver* solver, bool anisotropy, bool density)
{
	// use VBO buffer wrappers to allow Flex to write directly to the OpenGL buffers
	// Flex will take care of any CUDA interop mapping/unmapping during the get() operations
	if (!anisotropy)
	{
		// regular particles
		NvFlexGetParticles(solver, buffers.mPositionBuf, buffers.mNumFluidParticles);
	} else
	{
		// fluid buffers
		NvFlexGetSmoothParticles(solver, buffers.mPositionBuf, buffers.mNumFluidParticles);
		NvFlexGetAnisotropy(solver, buffers.mAnisotropyBuf[0], buffers.mAnisotropyBuf[1], buffers.mAnisotropyBuf[2]);
	}

	if (density)
	{
		NvFlexGetDensities(solver, buffers.mDensitiesBuf, buffers.mNumFluidParticles);
	} else
	{
		NvFlexGetPhases(solver, buffers.mDensitiesBuf, buffers.mNumFluidParticles);
	}

	NvFlexGetActive(solver, buffers.mIndicesBuf);
}

//---------------------------------------------------------------

void FlexUtil::UpdateFluidRenderBuffers(FlexFluidRenderBuffers buffers, glm::vec4* particles, float* densities,
		glm::vec4* anisotropy1, glm::vec4* anisotropy2, glm::vec4* anisotropy3, int numParticles, int* indices, int numIndices)
{
	// regular particles
	glBindBuffer(GL_ARRAY_BUFFER, buffers.mPositionVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, buffers.mNumFluidParticles*sizeof(glm::vec4), particles);

	if (anisotropy1)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers.mAnisotropyVBO[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, buffers.mNumFluidParticles*sizeof(glm::vec4), anisotropy1);
	}

	if (anisotropy2)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers.mAnisotropyVBO[1]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, buffers.mNumFluidParticles*sizeof(glm::vec4), anisotropy2);
	}

	if (anisotropy3)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers.mAnisotropyVBO[2]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, buffers.mNumFluidParticles*sizeof(glm::vec4), anisotropy3);
	}

	// density /phase buffer
	if (densities)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers.mDensityVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, buffers.mNumFluidParticles*sizeof(float), densities);
	}

	if (indices)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.mIndices);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, numIndices*sizeof(int), indices);
	}

	// reset
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

}

//---------------------------------------------------------------

void FlexUtil::UpdateDiffuseRenderBuffers(FlexDiffuseRenderBuffers buffers, NvFlexSolver* solver)
{
	// diffuse particles
	if (buffers.mNumDiffuseParticles)
	{
		NvFlexGetDiffuseParticles(solver, buffers.mDiffusePositionsBuf, buffers.mDiffuseVelocitiesBuf, buffers.mDiffuseIndicesBuf);
	}
}

//---------------------------------------------------------------

void FlexUtil::UpdateDiffuseRenderBuffers(FlexDiffuseRenderBuffers buffers, glm::vec4* diffusePositions, glm::vec4* diffuseVelocities,
		int* diffuseIndices, int numDiffuseParticles)
{
	// diffuse particles
	if (buffers.mNumDiffuseParticles)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.mDiffuseIndicesIBO);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, buffers.mNumDiffuseParticles*sizeof(int), diffuseIndices);

		glBindBuffer(GL_ARRAY_BUFFER, buffers.mDiffusePositionVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, buffers.mNumDiffuseParticles*sizeof(glm::vec4), diffusePositions);

		glBindBuffer(GL_ARRAY_BUFFER, buffers.mDiffuseVelocityVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, buffers.mNumDiffuseParticles*sizeof(glm::vec4), diffuseVelocities);
	}
}

//---------------------------------------------------------------

void FlexUtil::ErrorCallback(NvFlexErrorSeverity, const char* msg, const char* file, int line)
{
	printf("Flex: %s - %s:%d\n", msg, file, line);
	FlexUtil::flex_Error = true;
}

//---------------------------------------------------------------

void FlexUtil::CreateParticleShape(const FlexMesh* srcMesh, glm::vec3 lower, glm::vec3 scale, float rotation, float spacing,
		glm::vec3 velocity, float invMass, bool rigid, float rigidStiffness, int phase, bool skin,
		float jitter, glm::vec3 skinOffset, float skinExpand, glm::vec4 color,
		float springStiffness)
{
	if (rigid && g_buffers->rigidIndices.empty())
		g_buffers->rigidOffsets.push_back(0);

	if (!srcMesh)
		return;

	// duplicate mesh
	FlexMesh mesh;
	mesh.AddMesh(*srcMesh);

	int startIndex = int(g_buffers->positions.size());

	{
		mesh.Transform( glm::rotate(rotation, glm::vec3(0.0f, 1.0f, 0.0f)) );

		glm::vec3 meshLower, meshUpper;
		mesh.GetBounds(meshLower, meshUpper);

		glm::vec3 edges = meshUpper-meshLower;
		float maxEdge = glm::max(glm::max(edges.x, edges.y), edges.z);

		// put mesh at the origin and scale to specified size
		glm::mat4 xform = glm::scale(scale/maxEdge) * glm::translate(glm::vec3(-meshLower));

		mesh.Transform(xform);
		mesh.GetBounds(meshLower, meshUpper);

		// recompute expanded edges
		edges = meshUpper-meshLower;
		maxEdge = glm::max(glm::max(edges.x, edges.y), edges.z);

		// tweak spacing to avoid edge cases for particles laying on the boundary
		// just covers the case where an edge is a whole multiple of the spacing.
		float spacingEps = spacing*(1.0f - 1e-4f);

		// make sure to have at least one particle in each dimension
		int dx, dy, dz;
		dx = spacing > edges.x ? 1 : int(edges.x/spacingEps);
		dy = spacing > edges.y ? 1 : int(edges.y/spacingEps);
		dz = spacing > edges.z ? 1 : int(edges.z/spacingEps);

		int maxDim = glm::max(glm::max(dx, dy), dz);

		// expand border by two voxels to ensure adequate sampling at edges
		meshLower -= 2.0f*glm::vec3(spacing);
		meshUpper += 2.0f*glm::vec3(spacing);
		maxDim += 4;

		std::vector<uint32_t> voxels(maxDim*maxDim*maxDim);

		// we shift the voxelization bounds so that the voxel centers
		// lie symmetrically to the center of the object. this reduces the
		// chance of missing features, and also better aligns the particles
		// with the mesh
		glm::vec3 meshOffset;
		meshOffset.x = 0.5f * (spacing - (edges.x - (dx-1)*spacing));
		meshOffset.y = 0.5f * (spacing - (edges.y - (dy-1)*spacing));
		meshOffset.z = 0.5f * (spacing - (edges.z - (dz-1)*spacing));
		meshLower -= meshOffset;

		//Voxelize(*mesh, dx, dy, dz, &voxels[0], meshLower - glm::vec3(spacing*0.05f) , meshLower + glm::vec3(maxDim*spacing) + glm::vec3(spacing*0.05f));
		Voxelize((const float*)&mesh.m_positions[0],
				mesh.m_positions.size(),
				(const int*)&mesh.m_indices[0],
				mesh.m_indices.size(),
				maxDim,
				maxDim,
				maxDim,
				&voxels[0],
				meshLower,
				meshLower + glm::vec3(maxDim*spacing));

		std::vector<int> indices(maxDim*maxDim*maxDim);
		std::vector<float> sdf(maxDim*maxDim*maxDim);
		MakeSDF(&voxels[0], maxDim, maxDim, maxDim, &sdf[0]);

		for (int x=0; x < maxDim; ++x)
		{
			for (int y=0; y < maxDim; ++y)
			{
				for (int z=0; z < maxDim; ++z)
				{
					const int index = z*maxDim*maxDim + y*maxDim + x;

					// if voxel is marked as occupied the add a particle
					if (voxels[index])
					{
						if (rigid)
							g_buffers->rigidIndices.push_back(int(g_buffers->positions.size()));

						glm::vec3 position = lower + meshLower + spacing*glm::vec3(float(x) + 0.5f, float(y) + 0.5f, float(z) + 0.5f) + RandomUnitVector()*jitter;

						 // normalize the sdf value and transform to world scale
						glm::vec3 n = glm::normalize(SampleSDFGrad(&sdf[0], maxDim, x, y, z));
						float d = sdf[index]*maxEdge;

						if (rigid)
							g_buffers->rigidLocalNormals.push_back(glm::vec4(n, d));

						// track which particles are in which cells
						indices[index] = g_buffers->positions.size();

						g_buffers->positions.push_back(glm::vec4(position.x, position.y, position.z, invMass));
						g_buffers->velocities.push_back(velocity);
						g_buffers->phases.push_back(phase);
					}
				}
			}
		}
		mesh.Transform(ScaleMatrix(glm::vec3(1.0f + skinExpand)) * TranslationMatrix(glm::vec3(-0.5f*(meshUpper+meshLower))));
		mesh.Transform(TranslationMatrix(glm::vec3(lower + 0.5f*(meshUpper+meshLower))));


		if (springStiffness > 0.0f)
		{
			// construct cross link springs to occupied cells
			for (int x=0; x < maxDim; ++x)
			{
				for (int y=0; y < maxDim; ++y)
				{
					for (int z=0; z < maxDim; ++z)
					{
						const int centerCell = z*maxDim*maxDim + y*maxDim + x;

						// if voxel is marked as occupied the add a particle
						if (voxels[centerCell])
						{
							const int width = 1;

							// create springs to all the neighbors within the width
							for (int i=x-width; i <= x+width; ++i)
							{
								for (int j=y-width; j <= y+width; ++j)
								{
									for (int k=z-width; k <= z+width; ++k)
									{
										const int neighborCell = k*maxDim*maxDim + j*maxDim + i;

										if (neighborCell > 0 && neighborCell < int(voxels.size()) && voxels[neighborCell] && neighborCell != centerCell)
										{
											CreateSpring(indices[neighborCell], indices[centerCell], springStiffness);
										}
									}
								}
							}
						}
					}
				}
			}
		}

	}


	if (skin)
	{
		g_buffers->rigidMeshSize.push_back(mesh.GetNumVertices());

		int startVertex = 0;

		if (!g_mesh)
			g_mesh = new FlexMesh();

		// append to mesh
		startVertex = g_mesh->GetNumVertices();

		g_mesh->Transform(TranslationMatrix(glm::vec3(skinOffset)));
		g_mesh->AddMesh(mesh);

		const Colour colors[7] =
		{
			Colour(0.0f, 0.5f, 1.0f),
			Colour(0.797f, 0.354f, 0.000f),
			Colour(0.000f, 0.349f, 0.173f),
			Colour(0.875f, 0.782f, 0.051f),
			Colour(0.01f, 0.170f, 0.453f),
			Colour(0.673f, 0.111f, 0.000f),
			Colour(0.612f, 0.194f, 0.394f)
		};

		for (uint32_t i=startVertex; i < g_mesh->GetNumVertices(); ++i)
		{
			int indices[g_numSkinWeights] = { -1, -1, -1, -1 };
			float distances[g_numSkinWeights] = {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };

			if (glm::dot(color, color) == 0.0f)
				g_mesh->m_colours[i] = 1.25f*colors[phase%7];
			else
				g_mesh->m_colours[i] = Colour(color);

			// find closest n particles
			for (int j=startIndex; j < g_buffers->positions.size(); ++j)
			{
				glm::vec3 t = glm::vec3(g_mesh->m_positions[i]) - glm::vec3(g_buffers->positions[j]);
				float dSq = glm::dot(t, t);

				// insertion sort
				int w=0;
				for (; w < 4; ++w)
					if (dSq < distances[w])
						break;

				if (w < 4)
				{
					// shuffle down
					for (int s=3; s > w; --s)
					{
						indices[s] = indices[s-1];
						distances[s] = distances[s-1];
					}

					distances[w] = dSq;
					indices[w] = int(j);
				}
			}

			// weight particles according to distance
			float wSum = 0.0f;

			for (int w=0; w < 4; ++w)
			{
				// convert to inverse distance
				distances[w] = 1.0f/(0.1f + powf(distances[w], .125f));

				wSum += distances[w];

			}

			float weights[4];
			for (int w=0; w < 4; ++w)
				weights[w] = distances[w]/wSum;

			for (int j=0; j < 4; ++j)
			{
				g_meshSkinIndices.push_back(indices[j]);
				g_meshSkinWeights.push_back(weights[j]);
			}
		}
	}

	if (rigid)
	{
		g_buffers->rigidCoefficients.push_back(rigidStiffness);
		g_buffers->rigidOffsets.push_back(int(g_buffers->rigidIndices.size()));
	}
}

//---------------------------------------------------------------

void FlexUtil::CreateParticleShape(const char* filename, glm::vec3 lower, glm::vec3 scale, float rotation, float spacing,
		glm::vec3 velocity, float invMass, bool rigid, float rigidStiffness, int phase, bool skin, float jitter,
		glm::vec3 skinOffset, float skinExpand, glm::vec4 color, float springStiffness)
{
	FlexMesh* mesh = ImportMesh(filename);

	if (mesh)
		CreateParticleShape(mesh, lower, scale, rotation, spacing, velocity, invMass, rigid, rigidStiffness, phase,
				skin, jitter, skinOffset, skinExpand, color, springStiffness);

	delete mesh;
}

//---------------------------------------------------------------

void FlexUtil::CreateParticleGrid(glm::vec3 lower, int dimx, int dimy, int dimz, float radius, glm::vec3 velocity,
		float invMass, bool rigid, float rigidStiffness, int phase, float jitter)
{
	if (rigid && g_buffers->rigidIndices.empty())
		g_buffers->rigidOffsets.push_back(0);

	for (int x = 0; x < dimx; ++x)
	{
		for (int y = 0; y < dimy; ++y)
		{
			for (int z=0; z < dimz; ++z)
			{
				if (rigid)
					g_buffers->rigidIndices.push_back(int(g_buffers->positions.size()));

				glm::vec3 position = lower + glm::vec3(float(x), float(y), float(z))*radius + RandomUnitVector()*jitter;

				g_buffers->positions.push_back(glm::vec4(position.x, position.y, position.z, invMass));
				g_buffers->velocities.push_back(velocity);
				g_buffers->phases.push_back(phase);
			}
		}
	}

	if (rigid)
	{
		g_buffers->rigidCoefficients.push_back(rigidStiffness);
		g_buffers->rigidOffsets.push_back(int(g_buffers->rigidIndices.size()));
	}
}

//---------------------------------------------------------------

void FlexUtil::CreateSpring(int i, int j, float stiffness, float give)
{
	g_buffers->springIndices.push_back(i);
	g_buffers->springIndices.push_back(j);
	g_buffers->springLengths.push_back((1.0f+give)*glm::length(glm::vec3(g_buffers->positions[i])-glm::vec3(g_buffers->positions[j])));
	g_buffers->springStiffness.push_back(stiffness);
}

//---------------------------------------------------------------

void FlexUtil::CreateFluidRenderer(int width, int height, GLuint fbo)
{
	FlexFluidRenderer* renderer = new FlexFluidRenderer();

	renderer->mSceneWidth = width;
	renderer->mSceneHeight = height;

	/*
	// scene depth texture
	glGenTextures(1, &renderer->mDepthTex);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, renderer->mDepthTex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);

	getGlError();
	*/

	// smoothed depth texture
	glGenTextures(1, &renderer->mDepthSmoothTex);
	glBindTexture(GL_TEXTURE_2D, renderer->mDepthSmoothTex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);

	getGlError();

/*	// scene copy
	glGenTextures(1, &renderer->mSceneTex);
	glBindTexture(GL_TEXTURE_2D, renderer->mSceneTex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	getGlError();

	glGenFramebuffers(1, &renderer->mSceneFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, renderer->mSceneFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->mSceneTex, 0);
	getGlError();
*/
	sceneFbo = new FBO(shCol, width, height, GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	renderer->mSceneFbo = sceneFbo->getFbo();
	renderer->mSceneTex = sceneFbo->getColorImg(0);

/*
	glGenFramebuffers(1, &renderer->mDepthFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, renderer->mDepthFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, renderer->mDepthTex, 0);
	getGlError();

	GLuint zbuffer;
	glGenRenderbuffers(1, &zbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, zbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, zbuffer);
	getGlError();

	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	getGlError();
*/
	// frame buffer
	depthFbo = new FBO(shCol, width, height, GL_RGBA8, GL_TEXTURE_2D, true, 1, 1, 2, GL_CLAMP_TO_EDGE, false);
	renderer->mDepthFbo = depthFbo->getFbo();
	renderer->mDepthTex = depthFbo->getDepthImg();


	// reflect texture
	glGenTextures(1, &renderer->mReflectTex);
	glBindTexture(GL_TEXTURE_2D, renderer->mReflectTex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	getGlError();

	// thickness texture
	const int thicknessWidth = width;
	const int thicknessHeight = height;
	/*
	glGenTextures(1, &renderer->mThicknessTex);
	glBindTexture(GL_TEXTURE_2D, renderer->mThicknessTex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	getGlError();

#if USE_HDR_DIFFUSE_BLEND
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, thicknessWidth, thicknessHeight, 0, GL_RGBA, GL_FLOAT, NULL);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, thicknessWidth, thicknessHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
#endif

	// thickness buffer

	glGenFramebuffers(1, &renderer->mThicknessFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, renderer->mThicknessFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->mThicknessTex, 0);

	GLuint thickz;
	glGenRenderbuffers(1, &thickz);
	glBindRenderbuffer(GL_RENDERBUFFER, thickz);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, thicknessWidth, thicknessHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, thickz);

	glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	*/

#if USE_HDR_DIFFUSE_BLEND
	thickFbo = new FBO(shCol, thicknessWidth, thicknessHeight, GL_RGBA16F, GL_TEXTURE_2D, true, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
#else
	thickFbo = new FBO(shCol, thicknessWidth, thicknessHeight, GL_RGBA8, GL_TEXTURE_2D, true, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
#endif

	renderer->mThicknessFbo = thickFbo->getFbo();
	renderer->mThicknessTex = thickFbo->getColorImg(0);


	// compile shaders
	pointThickShdr = initPointThickShdr();
	renderer->mPointThicknessProgram = pointThickShdr->getProgram();

	ellipsoidDepthShdr = initEllipsoidDepthShdr();
	renderer->mEllipsoidDepthProgram = ellipsoidDepthShdr->getProgram();

	compositeShdr = initCompositeShdr();
	renderer->mCompositeProgram = compositeShdr->getProgram();

	depthBlurShdr = initDepthBlurShdr();
	renderer->mDepthBlurProgram = depthBlurShdr->getProgram();

	g_fluidRenderer = renderer;
}

//---------------------------------------------------------------

FlexFluidRenderBuffers FlexUtil::CreateFluidRenderBuffers(int numFluidParticles, bool enableInterop)
{
	printf("-- CreateFluidRenderBuffers\n");

	FlexFluidRenderBuffers buffers = {};
	buffers.mNumFluidParticles = numFluidParticles;

	// vbos
	glGenBuffers(1, &buffers.mPositionVBO);
	glBindBuffer(GL_ARRAY_BUFFER, buffers.mPositionVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * numFluidParticles, 0, GL_DYNAMIC_DRAW);

	// density
	glGenBuffers(1, &buffers.mDensityVBO);
	glBindBuffer(GL_ARRAY_BUFFER, buffers.mDensityVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(int)*numFluidParticles, 0, GL_DYNAMIC_DRAW);

	for (int i = 0; i < 3; ++i)
	{
		glGenBuffers(1, &buffers.mAnisotropyVBO[i]);
		glBindBuffer(GL_ARRAY_BUFFER, buffers.mAnisotropyVBO[i]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * numFluidParticles, 0, GL_DYNAMIC_DRAW);
	}

	glGenBuffers(1, &buffers.mIndices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.mIndices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*numFluidParticles, 0, GL_DYNAMIC_DRAW);

	if (enableInterop)
	{
		std::cout << " NvFlexLibrary* g_flexLib: " << g_flexLib << std::endl;

		buffers.mPositionBuf = NvFlexRegisterOGLBuffer(g_flexLib, buffers.mPositionVBO, numFluidParticles, sizeof(glm::vec4));
		buffers.mDensitiesBuf = NvFlexRegisterOGLBuffer(g_flexLib, buffers.mDensityVBO, numFluidParticles, sizeof(float));
		buffers.mIndicesBuf = NvFlexRegisterOGLBuffer(g_flexLib, buffers.mIndices, numFluidParticles, sizeof(int));

		buffers.mAnisotropyBuf[0] = NvFlexRegisterOGLBuffer(g_flexLib, buffers.mAnisotropyVBO[0], numFluidParticles, sizeof(glm::vec4));
		buffers.mAnisotropyBuf[1] = NvFlexRegisterOGLBuffer(g_flexLib, buffers.mAnisotropyVBO[1], numFluidParticles, sizeof(glm::vec4));
		buffers.mAnisotropyBuf[2] = NvFlexRegisterOGLBuffer(g_flexLib, buffers.mAnisotropyVBO[2], numFluidParticles, sizeof(glm::vec4));
	}

	return buffers;
}

//---------------------------------------------------------------

FlexDiffuseRenderBuffers FlexUtil::CreateDiffuseRenderBuffers(int numDiffuseParticles, bool& enableInterop)
{
	FlexDiffuseRenderBuffers buffers = {};
	buffers.mNumDiffuseParticles = numDiffuseParticles;

	if (numDiffuseParticles > 0)
	{
		glGenBuffers(1, &buffers.mDiffusePositionVBO);
		glBindBuffer(GL_ARRAY_BUFFER, buffers.mDiffusePositionVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * numDiffuseParticles, 0, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &buffers.mDiffuseVelocityVBO);
		glBindBuffer(GL_ARRAY_BUFFER, buffers.mDiffuseVelocityVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * numDiffuseParticles, 0, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &buffers.mDiffuseIndicesIBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.mDiffuseIndicesIBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*numDiffuseParticles, 0, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		if (enableInterop)
		{
			//extern NvFlexLibrary* g_flexLib;

			buffers.mDiffuseIndicesBuf = NvFlexRegisterOGLBuffer(g_flexLib, buffers.mDiffuseIndicesIBO, numDiffuseParticles, sizeof(int));
			buffers.mDiffusePositionsBuf = NvFlexRegisterOGLBuffer(g_flexLib, buffers.mDiffusePositionVBO, numDiffuseParticles, sizeof(glm::vec4));
			buffers.mDiffuseVelocitiesBuf = NvFlexRegisterOGLBuffer(g_flexLib, buffers.mDiffuseVelocityVBO, numDiffuseParticles, sizeof(glm::vec4));
		}
	}

	return buffers;
}

//---------------------------------------------------------------

Shaders* FlexUtil::initPointThickShdr()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert = STRINGIFY(
		layout( location = 0 ) in vec4 position;
		layout( location = 1 ) in vec4 normal;
		layout( location = 2 ) in vec2 texCoord;
		layout( location = 3 ) in vec4 color;
		uniform mat4 m_pvm;
		uniform mat4 m_vm;
		uniform float pointRadius;  // point size in world space
		uniform float pointScale;   // scale to calculate size in pixels
		out VS_FS {
			vec2 tex_coord;
			vec4 world_pos;
		} vertex_out;
		void main()
		{
			// calculate window-space point size
			gl_Position = m_pvm * vec4(position.xyz, 1.0);
			gl_PointSize = pointScale * (pointRadius / gl_Position.w);

			vertex_out.tex_coord = texCoord;
			vertex_out.world_pos = m_vm * vec4(position.xyz, 1.0);
		});
	vert = "// flexPointThick vertex shader\n" + shdr_Header + vert;


	std::string frag = STRINGIFY(
		in VS_FS {
			vec2 tex_coord;
			vec4 world_pos;
		} vertex_in;
		layout(location = 0) out vec4 fragColor;\n
		void main()
		{
			// calculate normal from texture coordinates
			vec3 normal;
			normal.xy = vertex_in.tex_coord * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
			float mag = dot(normal.xy, normal.xy);
			if (mag > 1.0) discard;   // kill pixels outside circle
			normal.z = sqrt(1.0-mag);

			fragColor = vec4(normal.z*0.005);
		});
	frag = "// flexPointThick shader\n" + shdr_Header + frag;

	return shCol->addCheckShaderText("flexPointThick", vert.c_str(), frag.c_str());
}

//---------------------------------------------------------------

Shaders* FlexUtil::initEllipsoidDepthShdr()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert = STRINGIFY(
			// rotation matrix in xyz, scale in w
			layout( location = 0 ) in vec4 position;\n
			layout( location = 1 ) in vec4 q1;\n
			layout( location = 2 ) in vec4 q2;\n
			layout( location = 3 ) in vec4 q3;\n

			uniform mat4 m_pvm;\n
			uniform mat4 m_vm;\n

			out VS_GS {\n
				vec4 tex_coord;\n
				mat4 mat;\n
				vec4 ndcPos;\n
			} vertex_out;\n

			// returns 1.0 for x==0.0 (unlike glsl)
			float Sign(float x) {\n
				return x < 0.0 ? -1.0: 1.0;\n
			}\n

			bool solveQuadratic(float a, float b, float c, out float minT, out float maxT)\n
			{\n
				if (a == 0.0 && b == 0.0)\n
				{\n
					minT = maxT = 0.0;\n
					return false;\n
				}\n

				float discriminant = b*b - 4.0*a*c;\n

				if (discriminant < 0.0)\n
				{\n
					return false;\n
				}\n

				float t = -0.5*(b + Sign(b)*sqrt(discriminant));\n
				minT = t / a;\n
				maxT = c / t;\n

				if (minT > maxT)\n
				{\n
					float tmp = minT;\n
					minT = maxT;\n
					maxT = tmp;\n
				}\n

				return true;\n
			}\n

			float DotInvW(vec4 a, vec4 b) {	\n
				return a.x*b.x + a.y*b.y + a.z*b.z - a.w*b.w;\n
			}\n

			void main()\n
			{
				vec3 worldPos = position.xyz;\n // - vec3(0.0, 0.1*0.25, 0.0);	// hack move towards ground to account for anisotropy

				// construct quadric matrix
				mat4 q;\n
				q[0] = vec4(q1.xyz * q1.w, 0.0);\n
				q[1] = vec4(q2.xyz * q2.w, 0.0);\n
				q[2] = vec4(q3.xyz * q3.w, 0.0);\n
				q[3] = vec4(worldPos, 1.0);\n

				// transforms a normal to parameter space (inverse transpose of (q*modelview)^-T)
				mat4 invClip = transpose(m_pvm * q);\n

				// solve for the right hand bounds in homogenous clip space
				float a1 = DotInvW(invClip[3], invClip[3]);\n
				float b1 = -2.0 * DotInvW(invClip[0], invClip[3]);\n
				float c1 = DotInvW(invClip[0], invClip[0]);\n

				float xmin;\n
				float xmax;\n
			 	solveQuadratic(a1, b1, c1, xmin, xmax);\n

				// solve for the right hand bounds in homogenous clip space
				float a2 = DotInvW(invClip[3], invClip[3]);\n
				float b2 = -2.0 * DotInvW(invClip[1], invClip[3]);\n
				float c2 = DotInvW(invClip[1], invClip[1]);\n

				float ymin;\n
				float ymax;\n
			 	solveQuadratic(a2, b2, c2, ymin, ymax);\n

				gl_Position = vec4(worldPos.xyz, 1.0);\n
				vertex_out.tex_coord = vec4(xmin, xmax, ymin, ymax);\n

				// construct inverse quadric matrix (used for ray-casting in parameter space)
				mat4 invq;\n
				invq[0] = vec4(q1.xyz/q1.w, 0.0);\n
				invq[1] = vec4(q2.xyz/q2.w, 0.0);\n
				invq[2] = vec4(q3.xyz/q3.w, 0.0);\n
				invq[3] = vec4(0.0, 0.0, 0.0, 1.0);\n

				invq = transpose(invq);\n
				invq[3] = -(invq * gl_Position);\n

				// transform a point from view space to parameter space
				invq = invq * m_vm;\n
				vertex_out.mat = invq;\n

				// compute ndc pos for frustrum culling in GS
				vec4 ndcPos = m_pvm * vec4(worldPos.xyz, 1.0);\n
				vertex_out.ndcPos = ndcPos / ndcPos.w;\n
			}
			);
	vert = "// flexEllipsoid vertex shader\n" + shdr_Header + vert;


	std::string geom = STRINGIFY(
			in VS_GS {\n
				vec4 tex_coord;\n
				mat4 mat;\n
				vec4 ndcPos;\n
			} vertex_in[];\n

			out GS_FS {\n
				mat4 mat;\n
				vec4 ndcPos;\n
			} vertex_out;\n

			void main()\n
			{\n
				vec3 pos = gl_in[0].gl_Position.xyz;\n
				vec4 bounds = vertex_in[0].tex_coord;\n
				vec4 ndcPos = vertex_in[0].ndcPos;\n

				// frustrum culling
				const float ndcBound = 1.0;\n
				if (ndcPos.x < -ndcBound) return;\n
				if (ndcPos.x > ndcBound) return;\n
				if (ndcPos.y < -ndcBound) return;\n
				if (ndcPos.y > ndcBound) return;\n
				\n
				float xmin = bounds.x;\n
				float xmax = bounds.y;\n
				float ymin = bounds.z;\n
				float ymax = bounds.w;\n
				\n
				// inv quadric transform
				vertex_out.mat = vertex_in[0].mat;\n
				\n
				gl_Position = vec4(xmin, ymax, 0.0, 1.0);\n
				EmitVertex();\n
				\n
				gl_Position = vec4(xmin, ymin, 0.0, 1.0);\n
				EmitVertex();\n
				\n
				gl_Position = vec4(xmax, ymax, 0.0, 1.0);\n
				EmitVertex();\n
				\n
				gl_Position = vec4(xmax, ymin, 0.0, 1.0);\n
				EmitVertex();\n
			}
			);
    shdr_Header = "#version 410 core\n#pragma optimize(on)\nlayout(points) in;\nlayout(triangle_strip, max_vertices = 4) out;\n";
	geom = "// flexEllipsoid geom shader\n" + shdr_Header + geom;


    shdr_Header = "#version 410 core\n";
	std::string frag = STRINGIFY(
			layout(location = 0) out vec4 fragColor;\n

			in GS_FS {
				mat4 mat;
				vec4 ndcPos;
			} vertex_in;

			uniform mat4 m_p_inv;
			uniform mat4 m_p;

			uniform vec3 invViewport;
			uniform vec3 invProjection;

			float Sign(float x) {
				return x < 0.0 ? -1.0: 1.0;
			}

			bool solveQuadratic(float a, float b, float c, out float minT, out float maxT)
			{
				if (a == 0.0 && b == 0.0)
				{
					minT = maxT = 0.0;
					return true;
				}

				float discriminant = b*b - 4.0*a*c;

				if (discriminant < 0.0)
				{
					return false;
				}

				float t = -0.5*(b + Sign(b)*sqrt(discriminant));
				minT = t / a;
				maxT = c / t;

				if (minT > maxT)
				{
					float tmp = minT;
					minT = maxT;
					maxT = tmp;
				}

				return true;
			}

			float sqr(float x) { return x*x; }

			void main()
			{
				// transform from view space to parameter space
				mat4 invQuadric = vertex_in.mat;
//				invQuadric[0] = gl_TexCoord[0];
//				invQuadric[1] = gl_TexCoord[1];
//				invQuadric[2] = gl_TexCoord[2];
//				invQuadric[3] = gl_TexCoord[3];

				vec4 ndcPos = vec4(gl_FragCoord.xy * invViewport.xy * vec2(2.0, 2.0) - vec2(1.0, 1.0), -1.0, 1.0);
				vec4 viewDir = m_p_inv * ndcPos;

				// ray to parameter space
				vec4 dir = invQuadric * vec4(viewDir.xyz, 0.0);
				vec4 origin = invQuadric[3];

				// set up quadratric equation
				float a = sqr(dir.x) + sqr(dir.y) + sqr(dir.z);// - sqr(dir.w);
				float b = dir.x*origin.x + dir.y*origin.y + dir.z*origin.z - dir.w*origin.w;
				float c = sqr(origin.x) + sqr(origin.y) + sqr(origin.z) - sqr(origin.w);

				float minT;
				float maxT;

				if (solveQuadratic(a, 2.0*b, c, minT, maxT))
				{
					vec3 eyePos = viewDir.xyz * minT;
					vec4 ndcPos = m_p * vec4(eyePos, 1.0);
					ndcPos.z /= ndcPos.w;

					fragColor = vec4(eyePos.z, 1.0, 1.0, 1.0);
					fragColor.z = ndcPos.z*0.5 + 0.5; // depth buffer
					return;
				}
				else
					discard;

				fragColor = vec4(0.5, 0.0, 0.0, 1.0);
			}
			);
	frag = "// flexEllipsoid frag shader\n" + shdr_Header + frag;

	return shCol->addCheckShaderText("flexEllipsoid", vert.c_str(), geom.c_str(), frag.c_str());
}

//---------------------------------------------------------------

Shaders* FlexUtil::initCompositeShdr()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert = STRINGIFY(
		layout( location = 0 ) in vec4 position;\n
		layout( location = 2 ) in vec2 texCoord;\n
		out vec2 tex_coord;\n
		void main()\n
		{\n
			tex_coord = texCoord;\n
			gl_Position = vec4(position.xyz, 1.0);\n
		});
	vert = "// flexComposite vertex shader\n" + shdr_Header + vert;


	std::string frag = STRINGIFY(
		layout(location = 0) out vec4 fragColor;\n
		\n
		in vec2 tex_coord;\n
		\n
		uniform sampler2D tex;\n
		uniform vec2 invTexScale;\n
		uniform vec3 lightPos;\n
		uniform vec3 lightDir;\n
		uniform float spotMin;\n
		uniform float spotMax;\n
		uniform vec4 color;\n
		uniform float ior;\n
		\n
		uniform vec2 clipPosToEye;\n
		\n
		uniform sampler2D reflectTex;\n
		uniform sampler2DShadow shadowTex;\n
		uniform vec2 shadowTaps[12];\n
		\n
		uniform mat4 lightTransform;\n
		uniform mat4 m_p;\n
		uniform mat4 m_vm;\n
		uniform mat4 m_vm_inv;\n
		\n
		uniform sampler2D thicknessTex;\n
		uniform sampler2D sceneTex;\n
		\n
		uniform bool debug;\n
		\n
		// sample shadow map
		float shadowSample(vec3 worldPos, out float attenuation)\n
		{\n
			// hack: ENABLE_SIMPLE_FLUID
			//attenuation = 0.0f;
			//return 0.5;
			\n
			vec4 pos = lightTransform * vec4(worldPos + lightDir * 0.15, 1.0);\n
			pos /= pos.w;\n
			vec3 uvw = (pos.xyz * 0.5) + vec3(0.5);\n
			\n
			attenuation = max(smoothstep(spotMax, spotMin, dot(pos.xy, pos.xy)), 0.05);\n
			\n
			// user clip
			if (uvw.x  < 0.0 || uvw.x > 1.0)\n
				return 1.0;\n
			if (uvw.y < 0.0 || uvw.y > 1.0)\n
				return 1.0;\n
				\n
			float s = 0.0;\n
			float radius = 0.002;\n
			\n
			for (int i=0; i < 8; i++)\n
			{\n
				s += textureProj(shadowTex, vec4(uvw.xy + shadowTaps[i] * radius, uvw.z, 1.0));\n
			}\n
			\n
			s /= 8.0;\n
			return s;\n
		}\n
		\n
		vec3 viewportToEyeSpace(vec2 coord, float eyeZ)\n
		{\n
			// find position at z=1 plane
			vec2 uv = (coord*2.0 - vec2(1.0)) * clipPosToEye;\n

			return vec3(-uv * eyeZ, eyeZ);\n
		}\n

		vec3 srgbToLinear(vec3 c) {\n
			return pow(c, vec3(2.2));\n
		}\n
		vec3 linearToSrgb(vec3 c) {\n
			return pow(c, vec3(1.0/2.2));\n
		}\n

		float sqr(float x) { return x*x; }\n
		float cube(float x) { return x*x*x; }\n

		void main()\n
		{\n
			float eyeZ = texture(tex, tex_coord).x;\n

			if (eyeZ == 0.0)\n
				discard;\n

			// reconstruct eye space pos from depth
			vec3 eyePos = viewportToEyeSpace(tex_coord, eyeZ);\n

			// finite difference approx for normals, can't take dFdx because
			// the one-sided difference is incorrect at shape boundaries
			vec3 zl = eyePos - viewportToEyeSpace(tex_coord - vec2(invTexScale.x, 0.0), texture(tex, tex_coord - vec2(invTexScale.x, 0.0)).x);\n
			vec3 zr = viewportToEyeSpace(tex_coord + vec2(invTexScale.x, 0.0), texture(tex, tex_coord + vec2(invTexScale.x, 0.0)).x) - eyePos;\n
			vec3 zt = viewportToEyeSpace(tex_coord + vec2(0.0, invTexScale.y), texture(tex, tex_coord + vec2(0.0, invTexScale.y)).x) - eyePos;\n
			vec3 zb = eyePos - viewportToEyeSpace(tex_coord - vec2(0.0, invTexScale.y), texture(tex, tex_coord - vec2(0.0, invTexScale.y)).x);\n

			vec3 dx = zl;\n
			vec3 dy = zt;\n

			if (abs(zr.z) < abs(zl.z))\n
				dx = zr;\n

			if (abs(zb.z) < abs(zt.z))\n
				dy = zb;\n

			//vec3 dx = dFdx(eyePos.xyz);
			//vec3 dy = dFdy(eyePos.xyz);

			vec4 worldPos = m_vm_inv * vec4(eyePos, 1.0);\n

			float attenuation;\n
			float shadow = shadowSample(worldPos.xyz, attenuation);\n

			vec3 l = (m_vm * vec4(lightDir, 0.0)).xyz;\n
			vec3 v = -normalize(eyePos);\n

			vec3 n = normalize(cross(dx, dy));\n
			vec3 h = normalize(v + l);\n

			vec3 skyColor = vec3(0.1, 0.2, 0.4) * 1.2;\n
			vec3 groundColor = vec3(0.1, 0.1, 0.2);\n

			float fresnel = 0.1 + (1.0 - 0.1) * cube(1.0 - max(dot(n, v), 0.0));\n

			vec3 lVec = normalize(worldPos.xyz - lightPos);\n

			float ln = dot(l, n) * attenuation;\n

			vec3 rEye = reflect(-v, n).xyz;\n
			vec3 rWorld = (m_vm_inv * vec4(rEye, 0.0)).xyz;\n

			vec2 texScale = vec2(0.75, 1.0);\n	// to account for backbuffer aspect ratio (todo: pass in)

			float refractScale = ior * 0.025;\n
			float reflectScale = ior * 0.1;\n

			// attenuate refraction near ground (hack)
			refractScale *= smoothstep(0.1, 0.4, worldPos.y);\n

			vec2 refractCoord = tex_coord + n.xy * refractScale * texScale;\n
			//vec2 refractCoord = gl_TexCoord[0].xy + refract(-v, n, 1.0/1.33)*refractScale*texScale;

			// read thickness from refracted coordinate otherwise we get halos around objectsw
			float thickness = max(texture(thicknessTex, refractCoord).x, 0.3);\n

			//vec3 transmission = exp(-(vec3(1.0)-color.xyz)*thickness);
			vec3 transmission = (1.0 - (1.0 - color.xyz) * thickness * 0.8) * color.w;\n
			vec3 refract = texture(sceneTex, refractCoord).xyz * transmission;\n

			vec2 sceneReflectCoord = tex_coord - rEye.xy * texScale * reflectScale / eyePos.z;\n
			vec3 sceneReflect = texture(sceneTex, sceneReflectCoord).xyz * shadow;\n

			vec3 planarReflect = texture(reflectTex, tex_coord).xyz;\n
			planarReflect = vec3(0.0);\n

			// fade out planar reflections above the ground
			vec3 reflect = mix(planarReflect, sceneReflect, smoothstep(0.05, 0.3, worldPos.y))\n
					+ mix(groundColor, skyColor, smoothstep(0.15, 0.25, rWorld.y) * shadow);\n

			// lighting
			vec3 diffuse = color.xyz * mix(vec3(0.29, 0.379, 0.59), vec3(1.0), (ln*0.5 + 0.5) * max(shadow, 0.4)) * (1.0 - color.w);\n
			vec3 specular = vec3(1.2 * pow(max(dot(h, n), 0.0), 400.0));\n

			fragColor.xyz = diffuse + (mix(refract, reflect, fresnel) + specular)*color.w;\n
			fragColor.w = 1.0;\n

			if (debug)\n
				fragColor = vec4(n * 0.5 + vec3(0.5), 1.0);\n

			// write valid z
			vec4 clipPos = m_p * vec4(0.0, 0.0, eyeZ, 1.0);\n
			clipPos.z /= clipPos.w;\n

			fragColor.z = clipPos.z * 0.5 + 0.5;\n
		}
		);
	frag = "// flexComposite shader\n" + shdr_Header + frag;

	return shCol->addCheckShaderText("flexComposite", vert.c_str(), frag.c_str());
}

//---------------------------------------------------------------

Shaders* FlexUtil::initDepthBlurShdr()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert =STRINGIFY(
			layout( location = 0 ) in vec4 position;\n
			layout( location = 2 ) in vec2 texCoord;\n
			out vec2 tex_coord;\n
			void main()\n
			{\n
				tex_coord = texCoord;\n
				gl_Position = vec4(position.xyz, 1.0);\n
			});
	vert = "// flexDepthBlur vertex shader\n" + shdr_Header + vert;


	std::string frag = STRINGIFY(
			layout(location = 0) out vec4 fragColor;\n

			in vec2 tex_coord;

			uniform sampler2D depthTex;
			uniform sampler2D thicknessTex;
			uniform float blurRadiusWorld;
			uniform float blurScale;
			uniform float blurFalloff;
			uniform vec2 invTexScale;

			uniform bool debug;

			float sqr(float x) { return x*x; }

			void main()
			{
			    // eye-space depth of center sample
			    float depth = texture(depthTex, gl_FragCoord.xy).x;
				float thickness = texture(thicknessTex, tex_coord).x;

				// hack: ENABLE_SIMPLE_FLUID
				//thickness = 0.0f;

				if (debug)
				{
					// do not blur
					fragColor.x = depth;
					return;
				}

				// threshold on thickness to create nice smooth silhouettes
				if (depth == 0.0)//|| thickness < 0.02f)
				{
					fragColor.x = 0.0;
					return;
				}

				float blurDepthFalloff = 5.5;//blurFalloff*mix(4.0, 1.0, thickness)/blurRadiusWorld*0.0375;	// these constants are just a re-scaling from some known good values

				float maxBlurRadius = 5.0;
				//float taps = min(maxBlurRadius, blurScale * (blurRadiusWorld / -depth));
				//vec2 blurRadius = min(mix(0.25, 2.0/blurFalloff, thickness) * blurScale * (blurRadiusWorld / -depth) / taps, 0.15)*invTexScale;

				//discontinuities between different tap counts are visible. to avoid this we
				//use fractional contributions between #taps = ceil(radius) and floor(radius)
				float radius = min(maxBlurRadius, blurScale * (blurRadiusWorld / -depth));
				float radiusInv = 1.0 / radius;
				float taps = ceil(radius);
				float frac = taps - radius;

				float sum = 0.0;
			    float wsum = 0.0;
				float count = 0.0;

			    for(float y=-taps; y <= taps; y += 1.0)
				{
			        for(float x=-taps; x <= taps; x += 1.0)
					{
						vec2 offset = vec2(x, y);

			            float ssample = texture(depthTex, gl_FragCoord.xy + offset).x;

						if (ssample < -10000.0*0.5)
							continue;

			            // spatial domain
			            float r1 = length(vec2(x, y)) * radiusInv;
						float w = exp(-(r1*r1));

						//float expectedDepth = depth + dot(vec2(dzdx, dzdy), offset);

			            // range domain (based on depth difference)
			            float r2 = (ssample - depth) * blurDepthFalloff;
			            float g = exp(-(r2*r2));

						//fractional radius contributions
						float wBoundary = step(radius, max(abs(x), abs(y)));
						float wFrac = 1.0 - wBoundary * frac;

						sum += ssample * w * g * wFrac;
						wsum += w * g * wFrac;
						count += g * wFrac;
			        }
			    }

			    if (wsum > 0.0) {
			        sum /= wsum;
			    }

				float blend = count / sqr(2.0 * radius + 1.0);
				fragColor.x = mix(depth, sum, blend);
			}
			);
	frag = "// flexDepthBlur shader\n" + shdr_Header + frag;

	return shCol->addCheckShaderText("flexDepthBlur", vert.c_str(), frag.c_str());
}

//---------------------------------------------------------------

float FlexUtil::SampleSDF(const float* sdf, int dim, int x, int y, int z)
{
	assert(x < dim && x >= 0);
	assert(y < dim && y >= 0);
	assert(z < dim && z >= 0);

	return sdf[z*dim*dim + y*dim + x];
}

//---------------------------------------------------------------

// return normal of signed distance field
glm::vec3 FlexUtil::SampleSDFGrad(const float* sdf, int dim, int x, int y, int z)
{
	int x0 = glm::max(x-1, 0);
	int x1 = glm::min(x+1, dim-1);

	int y0 = glm::max(y-1, 0);
	int y1 = glm::min(y+1, dim-1);

	int z0 = glm::max(z-1, 0);
	int z1 = glm::min(z+1, dim-1);

	float dx = (SampleSDF(sdf, dim, x1, y, z) - SampleSDF(sdf, dim, x0, y, z))*(dim*0.5f);
	float dy = (SampleSDF(sdf, dim, x, y1, z) - SampleSDF(sdf, dim, x, y0, z))*(dim*0.5f);
	float dz = (SampleSDF(sdf, dim, x, y, z1) - SampleSDF(sdf, dim, x, y, z0))*(dim*0.5f);

	return glm::vec3(dx, dy, dz);
}

//---------------------------------------------------------------

// calculates local space positions given a set of particles and rigid indices
void FlexUtil::CalculateRigidLocalPositions(const glm::vec4* restPositions, int numRestPositions, const int* offsets,
		const int* indices, int numRigids, glm::vec3* localPositions)
{

	// To improve the accuracy of the result, first transform the restPositions to relative coordinates (by finding the mean and subtracting that from all points)
	// Note: If this is not done, one might see ghost forces if the mean of the restPositions is far from the origin.

	// Calculate mean
	glm::vec3 shapeOffset(0.0f);

	for (int i = 0; i < numRestPositions; i++)
	{
		shapeOffset += glm::vec3(restPositions[i]);
	}

	shapeOffset /= float(numRestPositions);

	int count = 0;

	for (int r=0; r < numRigids; ++r)
	{
		const int startIndex = offsets[r];
		const int endIndex = offsets[r+1];

		const int n = endIndex-startIndex;

		assert(n);

		glm::vec3 com;

		for (int i=startIndex; i < endIndex; ++i)
		{
			const int r = indices[i];

			// By substracting meshOffset the calculation is done in relative coordinates
			com += glm::vec3(restPositions[r]) - shapeOffset;
		}

		com /= float(n);

		for (int i=startIndex; i < endIndex; ++i)
		{
			const int r = indices[i];

			// By substracting meshOffset the calculation is done in relative coordinates
			localPositions[count++] = (glm::vec3(restPositions[r]) - shapeOffset) - com;
		}
	}
}

//---------------------------------------------------------------

void FlexUtil::GetParticleBounds(glm::vec3& lower, glm::vec3& upper)
{
	lower = glm::vec3(FLT_MAX);
	upper = glm::vec3(-FLT_MAX);

	for (int i=0; i < g_buffers->positions.size(); ++i)
	{
		lower = glm::min(glm::vec3(g_buffers->positions[i]), lower);
		upper = glm::max(glm::vec3(g_buffers->positions[i]), upper);
	}
}

//---------------------------------------------------------------

// calculates the union bounds of all the collision shapes in the scene
void FlexUtil::GetShapeBounds(glm::vec3& totalLower, glm::vec3& totalUpper)
{
	Bounds totalBounds;

	for (int i=0; i < g_buffers->shapeFlags.size(); ++i)
	{
		NvFlexCollisionGeometry geo = g_buffers->shapeGeometry[i];

		int type = g_buffers->shapeFlags[i]&eNvFlexShapeFlagTypeMask;

		glm::vec3 localLower;
		glm::vec3 localUpper;

		switch(type)
		{
			case eNvFlexShapeBox:
			{
				localLower = -glm::vec3(geo.box.halfExtents[0], geo.box.halfExtents[1], geo.box.halfExtents[2]);
				localUpper = glm::vec3(geo.box.halfExtents[0], geo.box.halfExtents[1], geo.box.halfExtents[2]);
				break;
			}
			case eNvFlexShapeSphere:
			{
				localLower = -glm::vec3(geo.sphere.radius);
				localUpper = glm::vec3(geo.sphere.radius);
				break;
			}
			case eNvFlexShapeCapsule:
			{
				localLower = -glm::vec3(geo.capsule.halfHeight, 0.0f, 0.0f) - glm::vec3(geo.capsule.radius);
				localUpper = glm::vec3(geo.capsule.halfHeight, 0.0f, 0.0f) + glm::vec3(geo.capsule.radius);
				break;
			}
			case eNvFlexShapeConvexMesh:
			{
				NvFlexGetConvexMeshBounds(g_flexLib, geo.convexMesh.mesh, glm::value_ptr(localLower), glm::value_ptr(localUpper));

				// apply instance scaling
				localLower *= glm::vec3(geo.convexMesh.scale[0], geo.convexMesh.scale[1], geo.convexMesh.scale[2]);
				localUpper *= glm::vec3(geo.convexMesh.scale[0], geo.convexMesh.scale[1], geo.convexMesh.scale[2]);
				break;
			}
			case eNvFlexShapeTriangleMesh:
			{
				NvFlexGetTriangleMeshBounds(g_flexLib, geo.triMesh.mesh, glm::value_ptr(localLower), glm::value_ptr(localUpper));

				// apply instance scaling
				localLower *= glm::vec3(geo.triMesh.scale[0], geo.triMesh.scale[1], geo.triMesh.scale[2]);
				localUpper *= glm::vec3(geo.triMesh.scale[0], geo.triMesh.scale[1], geo.triMesh.scale[2]);
				break;
			}
			case eNvFlexShapeSDF:
			{
				localLower = glm::vec3(0.0f);
				localUpper = glm::vec3(geo.sdf.scale);
				break;
			}
		};

		// transform local bounds to world space
		glm::vec3 worldLower, worldUpper;
		TransformBounds(localLower, localUpper, glm::vec3(g_buffers->shapePositions[i]), g_buffers->shapeRotations[i], 1.0f, worldLower, worldUpper);

		totalBounds = Union(totalBounds, Bounds(worldLower, worldUpper));
	}

	totalLower = totalBounds.lower;
	totalUpper = totalBounds.upper;
}

//---------------------------------------------------------------

void FlexUtil::MapBuffers()
{
	FlexSimBuffers* buffers = g_buffers;

	buffers->positions.map();
	buffers->restPositions.map();
	buffers->velocities.map();
	buffers->phases.map();
	buffers->densities.map();
	buffers->anisotropy1.map();
	buffers->anisotropy2.map();
	buffers->anisotropy3.map();
	buffers->normals.map();
	buffers->diffusePositions.map();
	buffers->diffuseVelocities.map();
	buffers->diffuseIndices.map();
	buffers->smoothPositions.map();
	buffers->activeIndices.map();

	// convexes
	buffers->shapeGeometry.map();
	buffers->shapePositions.map();
	buffers->shapeRotations.map();
	buffers->shapePrevPositions.map();
	buffers->shapePrevRotations.map();
	buffers->shapeFlags.map();

	buffers->rigidOffsets.map();
	buffers->rigidIndices.map();
	buffers->rigidMeshSize.map();
	buffers->rigidCoefficients.map();
	buffers->rigidRotations.map();
	buffers->rigidTranslations.map();
	buffers->rigidLocalPositions.map();
	buffers->rigidLocalNormals.map();

	buffers->springIndices.map();
	buffers->springLengths.map();
	buffers->springStiffness.map();

	// inflatables
	buffers->inflatableTriOffsets.map();
	buffers->inflatableTriCounts.map();
	buffers->inflatableVolumes.map();
	buffers->inflatableCoefficients.map();
	buffers->inflatablePressures.map();

	buffers->triangles.map();
	buffers->triangleNormals.map();
	buffers->uvs.map();
}

//---------------------------------------------------------------

void FlexUtil::UnmapBuffers()
{
	FlexSimBuffers* buffers = g_buffers;

	// particles
	buffers->positions.unmap();
	buffers->restPositions.unmap();
	buffers->velocities.unmap();
	buffers->phases.unmap();
	buffers->densities.unmap();
	buffers->anisotropy1.unmap();
	buffers->anisotropy2.unmap();
	buffers->anisotropy3.unmap();
	buffers->normals.unmap();
	buffers->diffusePositions.unmap();
	buffers->diffuseVelocities.unmap();
	buffers->diffuseIndices.unmap();
	buffers->smoothPositions.unmap();
	buffers->activeIndices.unmap();

	// convexes
	buffers->shapeGeometry.unmap();
	buffers->shapePositions.unmap();
	buffers->shapeRotations.unmap();
	buffers->shapePrevPositions.unmap();
	buffers->shapePrevRotations.unmap();
	buffers->shapeFlags.unmap();

	// rigids
	buffers->rigidOffsets.unmap();
	buffers->rigidIndices.unmap();
	buffers->rigidMeshSize.unmap();
	buffers->rigidCoefficients.unmap();
	buffers->rigidRotations.unmap();
	buffers->rigidTranslations.unmap();
	buffers->rigidLocalPositions.unmap();
	buffers->rigidLocalNormals.unmap();

	// springs
	buffers->springIndices.unmap();
	buffers->springLengths.unmap();
	buffers->springStiffness.unmap();

	// inflatables
	buffers->inflatableTriOffsets.unmap();
	buffers->inflatableTriCounts.unmap();
	buffers->inflatableVolumes.unmap();
	buffers->inflatableCoefficients.unmap();
	buffers->inflatablePressures.unmap();

	// triangles
	buffers->triangles.unmap();
	buffers->triangleNormals.unmap();
	buffers->uvs.unmap();
}

//---------------------------------------------------------------

void FlexUtil::DestroyBuffers(FlexSimBuffers* buffers)
{
	// particles
	buffers->positions.destroy();
	buffers->restPositions.destroy();
	buffers->velocities.destroy();
	buffers->phases.destroy();
	buffers->densities.destroy();
	buffers->anisotropy1.destroy();
	buffers->anisotropy2.destroy();
	buffers->anisotropy3.destroy();
	buffers->normals.destroy();
	buffers->diffusePositions.destroy();
	buffers->diffuseVelocities.destroy();
	buffers->diffuseIndices.destroy();
	buffers->smoothPositions.destroy();
	buffers->activeIndices.destroy();

	// convexes
	buffers->shapeGeometry.destroy();
	buffers->shapePositions.destroy();
	buffers->shapeRotations.destroy();
	buffers->shapePrevPositions.destroy();
	buffers->shapePrevRotations.destroy();
	buffers->shapeFlags.destroy();

	// rigids
	buffers->rigidOffsets.destroy();
	buffers->rigidIndices.destroy();
	buffers->rigidMeshSize.destroy();
	buffers->rigidCoefficients.destroy();
	buffers->rigidRotations.destroy();
	buffers->rigidTranslations.destroy();
	buffers->rigidLocalPositions.destroy();
	buffers->rigidLocalNormals.destroy();

	// springs
	buffers->springIndices.destroy();
	buffers->springLengths.destroy();
	buffers->springStiffness.destroy();

	// inflatables
	buffers->inflatableTriOffsets.destroy();
	buffers->inflatableTriCounts.destroy();
	buffers->inflatableVolumes.destroy();
	buffers->inflatableCoefficients.destroy();
	buffers->inflatablePressures.destroy();

	// triangles
	buffers->triangles.destroy();
	buffers->triangleNormals.destroy();
	buffers->uvs.destroy();

	delete buffers;
}

//---------------------------------------------------------------

void FlexUtil::DestroyFluidRenderBuffers(FlexFluidRenderBuffers buffers)
{
	glDeleteBuffers(1, &buffers.mPositionVBO);
	glDeleteBuffers(3, buffers.mAnisotropyVBO);
	glDeleteBuffers(1, &buffers.mDensityVBO);
	glDeleteBuffers(1, &buffers.mIndices);

	NvFlexUnregisterOGLBuffer(buffers.mPositionBuf);
	NvFlexUnregisterOGLBuffer(buffers.mDensitiesBuf);
	NvFlexUnregisterOGLBuffer(buffers.mIndicesBuf);

	NvFlexUnregisterOGLBuffer(buffers.mAnisotropyBuf[0]);
	NvFlexUnregisterOGLBuffer(buffers.mAnisotropyBuf[1]);
	NvFlexUnregisterOGLBuffer(buffers.mAnisotropyBuf[2]);
}

//---------------------------------------------------------------

void FlexUtil::DestroyDiffuseRenderBuffers(FlexDiffuseRenderBuffers buffers)
{
	if (buffers.mNumDiffuseParticles > 0)
	{
		glDeleteBuffers(1, &buffers.mDiffusePositionVBO);
		glDeleteBuffers(1, &buffers.mDiffuseVelocityVBO);
		glDeleteBuffers(1, &buffers.mDiffuseIndicesIBO);

		NvFlexUnregisterOGLBuffer(buffers.mDiffuseIndicesBuf);
		NvFlexUnregisterOGLBuffer(buffers.mDiffusePositionsBuf);
		NvFlexUnregisterOGLBuffer(buffers.mDiffuseVelocitiesBuf);
	}
}

//---------------------------------------------------------------

void FlexUtil::DestroyGpuMesh(FlexGpuMesh* m)
{
	glDeleteBuffers(1, &m->mPositionsVBO);
	glDeleteBuffers(1, &m->mNormalsVBO);
	glDeleteBuffers(1, &m->mIndicesIBO);
}

//---------------------------------------------------------------

void FlexUtil::SkinMesh()
{
	if (g_mesh)
	{
		int startVertex = 0;

		for (int r=0; r < g_buffers->rigidRotations.size(); ++r)
		{
			const glm::mat3 rotation = glm::toMat3(g_buffers->rigidRotations[r]);
			const int numVertices = g_buffers->rigidMeshSize[r];

			for (int i=startVertex; i < numVertices+startVertex; ++i)
			{
				glm::vec3 skinPos;

				for (int w=0; w < 4; ++w)
				{
					// small shapes can have < 4 particles
					if (g_meshSkinIndices[i*4+w] > -1)
					{
						assert(g_meshSkinWeights[i*4+w] < FLT_MAX);

						int index = g_meshSkinIndices[i*4+w];
						float weight = g_meshSkinWeights[i*4+w];

						skinPos += (rotation*(g_meshRestPositions[i]-glm::vec3(g_buffers->restPositions[index])) + glm::vec3(g_buffers->positions[index]))*weight;
					}
				}

				g_mesh->m_positions[i] = glm::vec3(skinPos);
			}

			startVertex += numVertices;
		}

		g_mesh->CalculateNormals();
	}
}

//---------------------------------------------------------------

void FlexUtil::pushEmitter(FlexEmitter& e)
{
	g_emitters.push_back(e);
}

//---------------------------------------------------------------

void FlexUtil::setDiffuseScale(float v)
{
	g_diffuseScale = v;
}

//---------------------------------------------------------------

void FlexUtil::setDrawDiffuse(bool v)
{
	g_drawDiffuse = v;
}

//---------------------------------------------------------------

void FlexUtil::setDrawEllipsoids(bool v)
{
	g_drawEllipsoids = v;
}

//---------------------------------------------------------------

void FlexUtil::setDrawPoints(bool v)
{
	g_drawPoints = v;
}

//---------------------------------------------------------------

void FlexUtil::setFluidColor(glm::vec4 col)
{
	g_fluidColor = col;
}

//---------------------------------------------------------------

void FlexUtil::setLightDistance(float v)
{
	g_lightDistance = v;
}

//---------------------------------------------------------------

void FlexUtil::setMaxDiffuseParticles(float v)
{
	g_maxDiffuseParticles = v;
}

//---------------------------------------------------------------

void FlexUtil::setNumExtraParticles(unsigned int v)
{
	g_numExtraParticles = v;
}

//---------------------------------------------------------------

void FlexUtil::setNumSolidParticles(int val)
{
	g_numSolidParticles = val;
}

//---------------------------------------------------------------

void FlexUtil::setNumSubsteps(unsigned int v)
{
	g_numSubsteps = v;
}

//---------------------------------------------------------------

void FlexUtil::setSceneLower(glm::vec3 v)
{
	g_sceneLower = v;
}

//---------------------------------------------------------------

void FlexUtil::setWaveAmplitude(float v)
{
	g_waveAmplitude = v;
}

//---------------------------------------------------------------

void FlexUtil::setWaveFrequency(float v)
{
	g_waveFrequency = v;
}

//---------------------------------------------------------------

void FlexUtil::setWaveFloorTilt(float v)
{
	g_waveFloorTilt = v;
}

//---------------------------------------------------------------

double FlexUtil::getDt()
{
	return g_dt;
}

//---------------------------------------------------------------

FlexFluidRenderer* FlexUtil::getFluidRenderer()
{
	return g_fluidRenderer;
}

//---------------------------------------------------------------

float FlexUtil::getLightDistance()
{
	return g_lightDistance;
}

//---------------------------------------------------------------

unsigned int FlexUtil::getNumActPart()
{
	return NvFlexGetActiveCount(g_flex);
}

//---------------------------------------------------------------

unsigned int FlexUtil::getNumDiffPart()
{
	return NvFlexGetDiffuseParticles(g_flex, NULL, NULL, NULL);
}

//---------------------------------------------------------------

NvFlexParams* FlexUtil::getParams()
{
	return &g_params;
}

//---------------------------------------------------------------

NvFlexVector<glm::vec4>* FlexUtil::getSimBufPos()
{
	return &g_buffers->positions;
}

//---------------------------------------------------------------

FlexUtil::~FlexUtil()
{
}

} /* namespace tav */
