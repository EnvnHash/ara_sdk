//
// SNTestFlex.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestFlex.h"

#define STRINGIFY(A) #A

namespace tav
{

SNTestFlex::SNTestFlex(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "NoLight")
{
	osc = static_cast<OSCData*>(scd->osc);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	clearShader = shCol->getStdClear();

#ifdef HAVE_FLEX
	flex = new FlexUtil(shCol);
	setFlexParam(_scd);
	flex->warmup();
	g_shadowMap = ShadowCreate();
#endif
}

//---------------------------------------------------------------

#ifdef HAVE_FLEX
void SNTestFlex::setFlexParam(sceneData* _scd)
{
	flex->CreateParticleShape(((*_scd->dataPath)+"/models/flex/bunny.ply").c_str(),
			glm::vec3(4.0f, 0.0f, 0.0f), glm::vec3(0.5f), 0.0f, s,
			glm::vec3(0.0f, 0.0f, 0.0f),
			m, true, 1.0f, NvFlexMakePhase(group++, 0), true, 0.0f);
	/*
	flex->CreateParticleShape(((*_scd->dataPath)+"/models/flex/box.ply").c_str(),
				glm::vec3(4.0f, 0.0f, 1.0f), glm::vec3(0.45f), 0.0f, s,
				glm::vec3(0.0f, 0.0f, 0.0f), m, true, 1.0f, NvFlexMakePhase(group++, 0), true, 0.0f);

	flex->CreateParticleShape(((*_scd->dataPath)+"/models/flex/bunny.ply").c_str(),
				glm::vec3(3.0f, 0.0f, 0.0f), glm::vec3(0.5f), 0.0f, s,
				glm::vec3(0.0f, 0.0f, 0.0f), m, true, 1.0f, NvFlexMakePhase(group++, 0), true, 0.0f);

	flex->CreateParticleShape(((*_scd->dataPath)+"/models/flex/sphere.ply").c_str(),
				glm::vec3(3.0f, 0.0f, 1.0f), glm::vec3(0.45f), 0.0f, s,
				glm::vec3(0.0f, 0.0f, 0.0f), m, true, 1.0f, NvFlexMakePhase(group++, 0), true, 0.0f);

	flex->CreateParticleShape(((*_scd->dataPath)+"/models/flex/bunny.ply").c_str(),
				glm::vec3(2.0f, 0.0f, 1.0f), glm::vec3(0.5f), 0.0f, s,
				glm::vec3(0.0f, 0.0f, 0.0f), m, true, 1.0f, NvFlexMakePhase(group++, 0), true, 0.0f);

	flex->CreateParticleShape(((*_scd->dataPath)+"/models/flex/box.ply").c_str(),
				glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.45f), 0.0f, s,
				glm::vec3(0.0f, 0.0f, 0.0f), m, true, 1.0f, NvFlexMakePhase(group++, 0), true, 0.0f);
*/

	flex->setNumSolidParticles( flex->getSimBufPos()->size() );

	float restDistance = radius*0.55f;

//		if (mDam)
//		{
		flex->CreateParticleGrid(glm::vec3(0.0f, 0.0f, 0.6f), 24, 48, 24, restDistance, glm::vec3(0.0f), 1.0f, false, 0.0f,
				NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid), 0.005f);
		flex->setLightDistance( flex->getLightDistance() * 0.5f );
//		}

	flex->setSceneLower( glm::vec3(0.0f) );

	flex->setNumSubsteps(2);

	flex->getParams()->radius = radius;
	flex->getParams()->dynamicFriction = 0.01f;
	flex->getParams()->fluid = true;
	flex->getParams()->viscosity = 2.0f;
	flex->getParams()->numIterations = 4;
	flex->getParams()->vorticityConfinement = 40.0f;
	flex->getParams()->anisotropyScale = 30.0f;
	flex->getParams()->fluidRestDistance = restDistance;
	flex->getParams()->solidPressure = 0.f;
	flex->getParams()->relaxationFactor = 0.0f;
	flex->getParams()->cohesion = 0.02f;
	flex->getParams()->collisionDistance = 0.01f;

	flex->setMaxDiffuseParticles(64*1024);
	flex->setDiffuseScale(0.5f);

	flex->setFluidColor(glm::vec4(0.113f, 0.425f, 0.55f, 1.f));

	FlexEmitter e1;
	e1.mDir = glm::vec3(1.0f, 0.0f, 0.0f);
	e1.mRight = glm::vec3(0.0f, 0.0f, -1.0f);
	e1.mPos = glm::vec3(radius, 1.f, 0.65f);
	e1.mSpeed = (restDistance/flex->getDt())*2.0f; // 2 particle layers per-frame
	e1.mEnabled = true;

	flex->pushEmitter(e1);

	flex->setNumExtraParticles(48*1024);
	flex->setLightDistance(1.8f);
	flex->getParams()->numPlanes = 5;

	flex->setWaveFloorTilt(0.0f);
	flex->setWaveFrequency(1.5f);
	flex->setWaveAmplitude(2.0f);

	//flex->warmup = true;

	// draw options
	flex->setDrawPoints(false);
	flex->setDrawEllipsoids(true);
	flex->setDrawDiffuse(true);
}
#endif
  
//---------------------------------------------------------------

void SNTestFlex::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glClear (GL_DEPTH_BUFFER_BIT);   // necessary, sucks performance
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);   // necessary, sucks performance

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask (GL_FALSE);

	clearCol.r = osc->backColor;
	clearCol.g = osc->backColor;
	clearCol.b = osc->backColor;
	clearCol.a = 1.f - osc->feedback;

	clearShader->begin();
	clearShader->setIdentMatrix4fv("m_pvm");
	clearShader->setUniform4fv("clearCol", &clearCol[0], 1);
	_stdQuad->draw();

	//-------------------------------------------------------------------

#ifdef HAVE_FLEX
	if (!flex->getFluidRenderer())
	{
		int samples;
		glGetIntegerv(GL_MAX_SAMPLES_EXT, &samples);

		// clamp samples to 4 to avoid problems with point sprite scaling
		samples = glm::min(samples, glm::min(g_msaaSamples, 4));

		msaaFbo = new FBO(shCol, cp->actFboSize.x, cp->actFboSize.y, GL_RGBA8, GL_TEXTURE_2D, true, 1, 1, samples, GL_CLAMP_TO_EDGE, false);
		g_msaaFbo = msaaFbo->getFbo();
		g_msaaFbo = msaaFbo->getFbo();

		glEnable(GL_MULTISAMPLE);

		flex->CreateFluidRenderer(cp->actFboSize.x, cp->actFboSize.y, g_msaaFbo);
	}

	//-------------------------------------------------------------------
	// Scene Update

	flex->MapBuffers();
//	UpdateCamera();

	//-------------------------------------------------------------------
	// Render
/*
	if (g_profile && (!g_pause || g_step))
	{
		if (g_benchmark)
		{
			g_numDetailTimers = NvFlexGetDetailTimers(g_flex, &g_detailTimers);
		}
		else {
			memset(&g_timers, 0, sizeof(g_timers));
			NvFlexGetTimers(g_flex, &g_timers);
		}
	}

	float newSimLatency = NvFlexGetDeviceLatency(g_flex);
*/

	StartFrame();

	// main scene render
	RenderScene(cp);
//	RenderDebug();

	EndFrame(cp);

	flex->UnmapBuffers();
#endif
}

//---------------------------------------------------------------

#ifdef HAVE_FLEX

void SNTestFlex::StartFrame()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);

	glPointSize(5.0f);

	if (g_msaaFbo)
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_msaaFbo);
		glClearColor(0.f, 0.f, 0.f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
}

//---------------------------------------------------------------

void SNTestFlex::RenderScene(camPar* cp)
{
	//---------------------------------------
	// setup view and state

	float fov = float(M_PI) / 4.0f;
	float aspect = float(cp->actFboSize.x) / float(cp->actFboSize.y);

	glm::mat4 proj = glm::perspective( fov, aspect, 1.f, 200.f);
	glm::mat4 view = glm::rotate(-flex->g_camAngle.x, glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::rotate(-flex->g_camAngle.y, glm::vec3(std::cos(-flex->g_camAngle.x), 0.0f, std::sin(-flex->g_camAngle.x)))
			* glm::translate( -glm::vec3(flex->g_camPos) );

	//------------------------------------
	// lighting pass

	// expand scene bounds to fit most scenes
	flex->g_sceneLower = glm::min(flex->g_sceneLower, glm::vec3(-2.0f, 0.0f, -2.0f));
	flex->g_sceneUpper = glm::max(flex->g_sceneUpper, glm::vec3(2.0f, 2.0f, 2.0f));

	glm::vec3 sceneExtents = flex->g_sceneUpper -flex->g_sceneLower;
	glm::vec3 sceneCenter = 0.5f*(flex->g_sceneUpper + flex->g_sceneLower);

	lightDir = glm::normalize(glm::vec3(5.0f, 15.0f, 7.5f));
	lightPos = sceneCenter + lightDir * glm::length(sceneExtents) * flex->g_lightDistance;
	lightTarget = sceneCenter;

	// calculate tight bounds for shadow frustum
	float lightFov = 2.0f * std::atan(glm::length(flex->g_sceneUpper - sceneCenter) / glm::length(lightPos - sceneCenter));

	// scale and clamp fov for aesthetics
	lightFov = glm::clamp(lightFov, glm::radians(25.0f), glm::radians(65.0f));

	glm::mat4 lightPerspective = glm::perspective(glm::degrees(lightFov), 1.0f, 1.0f, 1000.0f);
	glm::mat4 lightView = glm::lookAt(glm::vec3(lightPos), glm::vec3(lightTarget), glm::vec3(0.f, 1.f, 0.f));
	glm::mat4 lightTransform = lightPerspective*lightView;

	// non-fluid particles maintain radius distance (not 2.0f*radius) so multiply by a half
	float radius = flex->g_params.solidRestDistance;

	// fluid particles overlap twice as much again, so half the radius again
	if (flex->g_params.fluid)
		radius = flex->g_params.fluidRestDistance;

	radius *= 0.5f;
	radius *= flex->g_pointScale;

	//-------------------------------------
	// shadowing pass

	if (flex->g_meshSkinIndices.size())
		flex->SkinMesh();


	// create shadow maps
	ShadowBegin(g_shadowMap);
	/*

	SetView(lightView, lightPerspective);
	SetCullMode(false);

	// give scene a chance to do custom drawing
	g_scenes[g_scene]->Draw(1);

	if (g_drawMesh)
		DrawMesh(g_mesh, g_meshColor);

	DrawShapes();

	if (g_drawCloth && g_buffers->triangles.size())
	{
		DrawCloth(&g_buffers->positions[0], &g_buffers->normals[0], g_buffers->uvs.size() ? &g_buffers->uvs[0].x : NULL, &g_buffers->triangles[0], g_buffers->triangles.size() / 3, g_buffers->positions.size(), 3, g_expandCloth);
	}

	if (g_drawRopes)
	{
		for (size_t i = 0; i < g_ropes.size(); ++i)
			DrawRope(&g_buffers->positions[0], &g_ropes[i].mIndices[0], g_ropes[i].mIndices.size(), radius*g_ropeScale, i);
	}

	int shadowParticles = numParticles;
	int shadowParticlesOffset = 0;

	if (!g_drawPoints)
	{
		shadowParticles = 0;

		if (g_drawEllipsoids && g_params.fluid)
		{
			shadowParticles = numParticles - g_numSolidParticles;
			shadowParticlesOffset = g_numSolidParticles;
		}
	}
	else
	{
		int offset = g_drawMesh ? g_numSolidParticles : 0;

		shadowParticles = numParticles - offset;
		shadowParticlesOffset = offset;
	}

	if (g_buffers->activeIndices.size())
		DrawPoints(g_fluidRenderBuffers.mPositionVBO, g_fluidRenderBuffers.mDensityVBO, g_fluidRenderBuffers.mIndices, shadowParticles, shadowParticlesOffset, radius, 2048, 1.0f, lightFov, g_lightPos, g_lightTarget, lightTransform, g_shadowMap, g_drawDensity);

	ShadowEnd();

	//----------------
	// lighting pass

	BindSolidShader(g_lightPos, g_lightTarget, lightTransform, g_shadowMap, 0.0f, glm::vec4(g_clearColor, g_fogDistance));

	SetView(view, proj);
	SetCullMode(true);

	DrawPlanes((glm::vec4*)g_params.planes, g_params.numPlanes, g_drawPlaneBias);

	if (g_drawMesh)
		DrawMesh(g_mesh, g_meshColor);


	DrawShapes();

	if (g_drawCloth && g_buffers->triangles.size())
		DrawCloth(&g_buffers->positions[0], &g_buffers->normals[0], g_buffers->uvs.size() ? &g_buffers->uvs[0].x : NULL, &g_buffers->triangles[0], g_buffers->triangles.size() / 3, g_buffers->positions.size(), 3, g_expandCloth);

	if (g_drawRopes)
	{
		for (size_t i = 0; i < g_ropes.size(); ++i)
			DrawRope(&g_buffers->positions[0], &g_ropes[i].mIndices[0], g_ropes[i].mIndices.size(), g_params.radius*0.5f*g_ropeScale, i);
	}

	// give scene a chance to do custom drawing
	g_scenes[g_scene]->Draw(0);

	UnbindSolidShader();


	// first pass of diffuse particles (behind fluid surface)
	if (g_drawDiffuse)
		RenderDiffuse(g_fluidRenderer, g_diffuseRenderBuffers, numDiffuse, radius*g_diffuseScale, float(g_screenWidth), aspect, fov, g_diffuseColor, g_lightPos, g_lightTarget, lightTransform, g_shadowMap, g_diffuseMotionScale, g_diffuseInscatter, g_diffuseOutscatter, g_diffuseShadow, false);

	if (g_drawEllipsoids && g_params.fluid)
	{
		// draw solid particles separately
		if (g_numSolidParticles && g_drawPoints)
			DrawPoints(g_fluidRenderBuffers.mPositionVBO, g_fluidRenderBuffers.mDensityVBO, g_fluidRenderBuffers.mIndices, g_numSolidParticles, 0, radius, float(g_screenWidth), aspect, fov, g_lightPos, g_lightTarget, lightTransform, g_shadowMap, g_drawDensity);

		// render fluid surface
		RenderEllipsoids(g_fluidRenderer, g_fluidRenderBuffers, numParticles - g_numSolidParticles, g_numSolidParticles, radius, float(g_screenWidth), aspect, fov, g_lightPos, g_lightTarget, lightTransform, g_shadowMap, g_fluidColor, g_blur, g_ior, g_drawOpaque);

		// second pass of diffuse particles for particles in front of fluid surface
		if (g_drawDiffuse)
			RenderDiffuse(g_fluidRenderer, g_diffuseRenderBuffers, numDiffuse, radius*g_diffuseScale, float(g_screenWidth), aspect, fov, g_diffuseColor, g_lightPos, g_lightTarget, lightTransform, g_shadowMap, g_diffuseMotionScale, g_diffuseInscatter, g_diffuseOutscatter, g_diffuseShadow, true);
	}
	else
	{
		// draw all particles as spheres
		if (g_drawPoints)
		{
			int offset = g_drawMesh ? g_numSolidParticles : 0;

			if (g_buffers->activeIndices.size())
				DrawPoints(g_fluidRenderBuffers.mPositionVBO, g_fluidRenderBuffers.mDensityVBO, g_fluidRenderBuffers.mIndices, numParticles - offset, offset, radius, float(g_screenWidth), aspect, fov, g_lightPos, g_lightTarget, lightTransform, g_shadowMap, g_drawDensity);
		}
	}
*/
}

//---------------------------------------------------------------

void SNTestFlex::ShadowBegin(ShadowMap* map)
{
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(8.f, 8.f);

	glBindFramebuffer(GL_FRAMEBUFFER, map->framebuffer);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, kShadowResolution, kShadowResolution);

	// draw back faces (for teapot)
	glDisable(GL_CULL_FACE);

	// bind shadow shader
	if (passTroughShdr == 0)
		initShadowPassTroughShdr();

	//glUseProgram(s_shadowProgram);
	passTroughShdr->begin();
	passTroughShdr->setIdentMatrix4fv("objectTransform");
}

//---------------------------------------------------------------

void SNTestFlex::EndFrame(camPar* cp)
{
	if (g_msaaFbo)
	{
		// blit the msaa buffer to the window
		glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, g_msaaFbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, 0);
		glBlitFramebuffer(0, 0, cp->actFboSize.x, cp->actFboSize.y, 0, 0,
				cp->actFboSize.x, cp->actFboSize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}

	// render help to back buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT);
}

#endif

//---------------------------------------------------------------

void SNTestFlex::update(double time, double dt)
{

#ifdef HAVE_FLEX

	// move mouse particle (must be done here as GetViewRay() uses the GL projection state)
//	if (g_mouseParticle != -1)
//	{
//		glm::vec3 origin, dir;
//		GetViewRay(g_lastx, g_screenHeight - g_lasty, origin, dir);
//
//		g_mousePos = origin + dir*g_mouseT;
//	}

	//-------------------------------------------------------------------
	// Flex Update

	flex->update();

	// allow scene to update constraints etc
	//SyncScene();
#endif
}

//---------------------------------------------------------------

#ifdef HAVE_FLEX

SNTestFlex::ShadowMap* SNTestFlex::ShadowCreate()
{
	GLuint texture;
	GLuint framebuffer;

	glGenFramebuffers(1, &framebuffer);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// This is to allow usage of shadow2DProj function in the shader
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
//	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY));

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, kShadowResolution, kShadowResolution, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);

	ShadowMap* map = new ShadowMap();
	map->texture = texture;
	map->framebuffer = framebuffer;

	return map;
}

//---------------------------------------------------------------

void SNTestFlex::UpdateCamera()
{
//	glm::vec3 forward(-std::sin(g_camAngle.x) * std::cos(g_camAngle.y), std::sin(g_camAngle.y), -std::cos(g_camAngle.x) * std::cos(g_camAngle.y));
//	glm::vec3 right(Normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f))));
//
//	g_camSmoothVel = Lerp(g_camSmoothVel, g_camVel, 0.1f);
//	g_camPos += (forward*g_camSmoothVel.z + right*g_camSmoothVel.x + glm::cross(right, forward)*g_camSmoothVel.y);
}

//---------------------------------------------------------------

void SNTestFlex::initShadowPassTroughShdr()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert =STRINGIFY(
			layout( location = 0 ) in vec4 position;\n
			layout( location = 1 ) in vec4 normal;\n
			layout( location = 2 ) in vec2 texCoord;\n
			layout( location = 3 ) in vec4 color;\n

			uniform mat4 lightTransform;
			uniform vec3 lightDir;
			uniform float bias;
			uniform vec4 clipPlane;
			uniform float expand;

			uniform mat4 objectTransform;
			uniform mat4 m_pvm;
			uniform mat4 m_mv;

			out VS_FS {
				vec3 normal;
				vec4 lightTrans;
				vec4 lightDir;
				vec3 modelPos;
				vec4 color;
				vec2 tex_coord;
				vec4 second_color;
				vec4 modelViewPos;
			} vertex_out;

			void main()
			{
				vec3 n = normalize((objectTransform * normal).xyz);
				vec3 p = (objectTransform * position).xyz;

			    // calculate window-space point size
				gl_Position = m_pvm * vec4(p + expand*n, 1.0);

				vertex_out.normal = n;
				vertex_out.lightTrans = lightTransform * vec4(p + n*bias, 1.0);
				vertex_out.lightDir = m_mv * vec4(lightDir, 0.0);
				vertex_out.modelPos = p;
				vertex_out.color = color;
				vertex_out.tex_coord = texCoord;
				//vertex_out.second_color = gl_SecondaryColor;
				vertex_out.modelViewPos = m_mv * position;

				//gl_ClipDistance[0] = dot(clipPlane, position);
			});
	vert = "// flexShadowMap vertex shader\n" + shdr_Header + vert;


	std::string frag = STRINGIFY(
			layout(location = 0) out vec4 fragColor;\n

			in VS_FS {
				vec3 normal;
				vec4 lightTrans;
				vec4 lightDir;
				vec3 modelPos;
				vec4 color;
				vec2 tex_coord;
				vec4 second_color;
				vec4 modelViewPos;
			} vertex_in;

			void main() {}
				fragColor = vec4(0.0, 0.0, 0.0, 1.0);
			}
			);
	frag = "// flexShadowMap frag shader\n" + shdr_Header + frag;

	passTroughShdr = shCol->addCheckShaderText("SNTextFlex_shadow", vert.c_str(), frag.c_str());
}

#endif

//---------------------------------------------------------------

SNTestFlex::~SNTestFlex()
{
}

}
