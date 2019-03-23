//
//  GLSLFluid3D.cpp
//  Tav_App
//
//  adapted from ofxFluid.cpp
//  Created by Patricio Gonzalez Vivo on 9/29/11
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "GLSLFluid3D.h"

namespace tav
{
GLSLFluid3D::GLSLFluid3D(ShaderCollector* _shCol) :
		shCol(_shCol), defaultThetaX(0), defaultThetaY(0.75f), fips(-4.f), fieldOfView(
				0.7f), numJacobiIterations(40)
{
	stdTexShader = _shCol->getStdTex();

	thetaX = defaultThetaX;
	thetaY = defaultThetaY;

	cellSize = 1.25f;
	gridSize = glm::ivec3(64, 64, 64);
	eyePosition = glm::vec3(0, 0, 2.f);

	viewSamples = gridSize.x * 2;
	lightSamples = gridSize.x;

	splatRadius = float(gridSize.x) / 8.f;
	ambientTemperature = 0.f;
	impulseTemperature = 10.f;
	impulseDensity = 1.25f;

	timeStep = 0.25f;
	smokeBuoyancy = 1.f;
	smokeWeight = 0.f;
	gradientScale = 1.125f / cellSize;
	temperatureDissipation = 0.99f;
	velocityDissipation = 0.99f;
	densityDissipation = 0.999f;

	impulsePosition = glm::vec3(float(gridSize.x) / 2.f,
			float(gridSize.y - int(splatRadius / 2.f)),
			float(gridSize.z) / 2.f);

	/*
	 applyVelTextureRadius = 0.08f;
	 gForce = glm::vec2(0,-0.98);
	 applyImpMultCol = glm::vec3(1.f, 1.f, 1.f);
	 whiteCol = glm::vec3(1.f, 1.f, 1.f);
	 */

	camMat = glm::scale(glm::mat4(1.f), glm::vec3(1.f, -1.f, 1.f));

	initCommonShdr();
	initRayCastShdr();
	initLightShdr();
	initBlurShdr();
	initAdvect();
	initJacobi();
	initSubtractGrad();
	initComputeDiv();
	initApplyImp();
	initApplyBuoyancy();

	CubeCenter = new VAO("position:3f", GL_DYNAMIC_DRAW);
	GLfloat initPos[3] = { 0.f, 0.f, 0.f };
	CubeCenter->initData(1, initPos);

	FullscreenQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	Slabs.Velocity = new PingPongFbo(_shCol, gridSize.x, gridSize.y, gridSize.z,
			GL_RGB16F, GL_TEXTURE_3D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	Slabs.Velocity->clear();


	Slabs.Density = new PingPongFbo(_shCol, gridSize.x, gridSize.y, gridSize.z,
			GL_R16F, GL_TEXTURE_3D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	Slabs.Density->clear();

	Slabs.Pressure = new PingPongFbo(_shCol, gridSize.x, gridSize.y, gridSize.z,
			GL_R16F, GL_TEXTURE_3D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	Slabs.Pressure->clear();

	Slabs.Temperature = new PingPongFbo(_shCol, gridSize.x, gridSize.y, gridSize.z,
			GL_R16F, GL_TEXTURE_3D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	Slabs.Temperature->clear();

	Surfaces.Divergence = new FBO(_shCol, gridSize.x, gridSize.y, gridSize.z,
			GL_RGB16F, GL_TEXTURE_3D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	Surfaces.Divergence->clear();

	Surfaces.LightCache = new FBO(_shCol, gridSize.x, gridSize.y, gridSize.z,
			GL_R16F, GL_TEXTURE_3D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	Surfaces.LightCache->clear();

	Surfaces.BlurredDensity = new FBO(_shCol, gridSize.x, gridSize.y, gridSize.z,
			GL_R16F, GL_TEXTURE_3D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	Surfaces.BlurredDensity->clear();

	Surfaces.Obstacles = new FBO(_shCol, gridSize.x, gridSize.y, gridSize.z,
			GL_RGB16F, GL_TEXTURE_3D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	Surfaces.Obstacles->clear();

	CreateObstacles(Surfaces.Obstacles);
	//  Slabs.Temperature->clearAlpha(1.f, ambientTemperature);

}

//------------------------------------------------------------------------------------

void GLSLFluid3D::update(double time)
{
	float dt = float(time) * 0.0001f;
	float _fips = 1.0f / dt;
	float alpha = 0.05f;
	if (fips < 0)
		fips++;
	else if (fips == 0)
		fips = _fips;
	else
		fips = _fips * alpha + fips * (1.0f - alpha);

	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	Advect(Slabs.Velocity->src, Slabs.Velocity->src, Surfaces.Obstacles,
			Slabs.Velocity->dst, velocityDissipation);
	Slabs.Velocity->swap();

	Advect(Slabs.Velocity->src, Slabs.Temperature->src, Surfaces.Obstacles,
			Slabs.Temperature->dst, temperatureDissipation);
	Slabs.Temperature->swap();

	Advect(Slabs.Velocity->src, Slabs.Density->src, Surfaces.Obstacles,
			Slabs.Density->dst, densityDissipation);
	Slabs.Density->swap();

	ApplyBuoyancy(Slabs.Velocity->src, Slabs.Temperature->src,
			Slabs.Density->src, Slabs.Velocity->dst);
	Slabs.Velocity->swap();

	glEnable(GL_BLEND);
	ApplyImpulse(Slabs.Temperature->src, impulsePosition, impulseTemperature);
	ApplyImpulse(Slabs.Density->src, impulsePosition, impulseDensity);
	glDisable(GL_BLEND);

	ComputeDivergence(Slabs.Velocity->src, Surfaces.Obstacles,
			Surfaces.Divergence);
	Slabs.Pressure->src->clear();

	for (int i = 0; i < numJacobiIterations; ++i)
	{
		Jacobi(Slabs.Pressure->src, Surfaces.Divergence, Surfaces.Obstacles,
				Slabs.Pressure->dst);
		Slabs.Pressure->swap();
	}

	SubtractGradient(Slabs.Velocity->src, Slabs.Pressure->src,
			Surfaces.Obstacles, Slabs.Velocity->dst);
	Slabs.Velocity->swap();
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::draw()
{
	glm::vec3 up = glm::vec3(1.f, 0.f, 0.f);
	glm::vec3 target = glm::vec3(0.f);

	Matrices.View = glm::lookAt(eyePosition, target, up);

	glm::mat4 modelMatrix = glm::mat4(1.f);
	modelMatrix = glm::rotate(modelMatrix, thetaX, glm::vec3(1.f, 0.f, 0.f));
	modelMatrix = glm::rotate(modelMatrix, thetaY, glm::vec3(0.f, 1.f, 0.f));

	Matrices.Modelview = Matrices.View * modelMatrix;
	Matrices.Projection = glm::perspective(fieldOfView, 1360.f / 768.f, // Aspect Ratio
	0.0f,   // Near Plane
			1.0f);  // Far Plane
	Matrices.ModelviewProjection = Matrices.Projection * Matrices.Modelview;

	// Blur and brighten the density map:
	bool BlurAndBrighten = true;
	if (BlurAndBrighten)
	{
		glDisable(GL_BLEND);

		Surfaces.BlurredDensity->bind();

		BlurShdr->begin();
		BlurShdr->setUniform1i("Density", 0);
		BlurShdr->setUniform1f("DensityScale", 5.0f);
		BlurShdr->setUniform1f("StepSize", sqrtf(2.0) / float(viewSamples));
		BlurShdr->setUniform3f("InverseSize", 1.f / float(gridSize.x),
				1.f / float(gridSize.y), 1.f / float(gridSize.z));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, Slabs.Density->src->getColorImg());

		FullscreenQuad->drawInstanced(GL_TRIANGLES, gridSize.z, nullptr, 1.f);

		Surfaces.BlurredDensity->unbind();
	}

	// Generate the light cache:
	bool CacheLights = true;
	if (CacheLights)
	{
		glDisable(GL_BLEND);

		Surfaces.LightCache->bind();

		LightShdr->begin();
		LightShdr->setUniform1i("Density", 0);
		LightShdr->setUniform1f("LightStep", sqrtf(2.0) / float(lightSamples));
		LightShdr->setUniform1i("LightSamples", lightSamples);
		LightShdr->setUniform3f("InverseSize", 1.f / float(gridSize.x),
				1.f / float(gridSize.y), 1.f / float(gridSize.z));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, Surfaces.BlurredDensity->getColorImg());

		FullscreenQuad->drawInstanced(GL_TRIANGLES, gridSize.z, nullptr, 1.f);

		Surfaces.LightCache->unbind();
	}

	// Perform raycasting:
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glm::vec3 ryOrig = glm::vec3(
			glm::transpose(Matrices.Modelview) * glm::vec4(eyePosition, 1.f));

	RaycastShdr->begin();
	RaycastShdr->setUniformMatrix4fv("ModelviewProjection",
			&Matrices.ModelviewProjection[0][0]);
	RaycastShdr->setUniformMatrix4fv("Modelview", &Matrices.Modelview[0][0]);
	RaycastShdr->setUniform3f("EyePosition", eyePosition.x, eyePosition.y,
			eyePosition.z);
	RaycastShdr->setUniform3fv("RayOrigin", &ryOrig[0]);
	RaycastShdr->setUniform1f("FocalLength", 1.f / std::tan(fieldOfView / 2));
	RaycastShdr->setUniform2f("WindowSize", 1360.f, 768.f);
	RaycastShdr->setUniform1f("StepSize", sqrtf(2.0) / float(viewSamples));
	RaycastShdr->setUniform1i("ViewSamples", viewSamples);

	RaycastShdr->setUniform1i("Density", 0);
	RaycastShdr->setUniform1i("LightCache", 1);

	glActiveTexture(GL_TEXTURE0);
	if (BlurAndBrighten)
		glBindTexture(GL_TEXTURE_3D, Surfaces.BlurredDensity->getColorImg());
	else
		glBindTexture(GL_TEXTURE_3D, Slabs.Density->src->getColorImg());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, Surfaces.LightCache->getColorImg());

	CubeCenter->draw(GL_POINTS);
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::Advect(FBO* velocity, FBO* source, FBO* obstacles, FBO* dest,
		float dissipation)
{
	dest->bind();

	AdvectShdr->begin();
	AdvectShdr->setUniform3f("InverseSize", 1.f / float(gridSize.x),
			1.f / float(gridSize.y), 1.f / float(gridSize.z));
	AdvectShdr->setUniform1f("TimeStep", timeStep);
	AdvectShdr->setUniform1f("Dissipation", dissipation);
	AdvectShdr->setUniform1i("Velocity", 0);
	AdvectShdr->setUniform1i("SourceTexture", 1);
	AdvectShdr->setUniform1i("Obstacles", 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, velocity->getColorImg());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, source->getColorImg());

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, obstacles->getColorImg());

	FullscreenQuad->drawInstanced(GL_TRIANGLES, gridSize.z, nullptr, 1.f);
	dest->unbind();
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::Jacobi(FBO* pressure, FBO* divergence, FBO* obstacles,
		FBO* dest)
{
	dest->bind();

	JacobiShdr->begin();
	JacobiShdr->setUniform1f("Alpha", -cellSize * cellSize);
	JacobiShdr->setUniform1f("InverseBeta", 0.1666f);
	JacobiShdr->setUniform1i("Pressure", 0);
	JacobiShdr->setUniform1i("Divergence", 1);
	JacobiShdr->setUniform1i("Obstacles", 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, pressure->getColorImg());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, divergence->getColorImg());

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, obstacles->getColorImg());

	FullscreenQuad->drawInstanced(GL_TRIANGLES, gridSize.z, nullptr, 1.f);

	dest->unbind();
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::SubtractGradient(FBO* velocity, FBO* pressure, FBO* obstacles,
		FBO* dest)
{
	dest->bind();

	SubtractGradientShdr->begin();

	SubtractGradientShdr->setUniform1f("GradientScale", gradientScale);
	SubtractGradientShdr->setUniform1f("HalfInverseCellSize", 0.5f / cellSize);
	SubtractGradientShdr->setUniform1i("Velocity", 0);
	SubtractGradientShdr->setUniform1i("Pressure", 1);
	SubtractGradientShdr->setUniform1i("Obstacles", 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, velocity->getColorImg());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, pressure->getColorImg());
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, obstacles->getColorImg());

	//glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, dest->getDepth());
	FullscreenQuad->drawInstanced(GL_TRIANGLES, gridSize.z, nullptr, 1.f);

	dest->unbind();
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::ComputeDivergence(FBO* velocity, FBO* obstacles, FBO* dest)
{
	dest->bind();

	ComputeDivergenceShdr->begin();
	ComputeDivergenceShdr->setUniform1f("HalfInverseCellSize", 0.5f / cellSize);
	ComputeDivergenceShdr->setUniform1i("Velocity", 0);
	ComputeDivergenceShdr->setUniform1i("Obstacles", 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, velocity->getColorImg());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, obstacles->getColorImg());

	FullscreenQuad->drawInstanced(GL_TRIANGLES, gridSize.z, nullptr, 1.f);

	dest->unbind();
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::ApplyImpulse(FBO* dest, glm::vec3 position, float value)
{
	dest->bind();

	ApplyImpulseShdr->begin();
	ApplyImpulseShdr->setUniform3fv("Point", &position[0]);
	ApplyImpulseShdr->setUniform1f("Radius", splatRadius);
	ApplyImpulseShdr->setUniform3f("FillColor", value, value, value);

	FullscreenQuad->drawInstanced(GL_TRIANGLES, gridSize.z, nullptr, 1.f);

	ApplyImpulseShdr->end();

	dest->unbind();
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::ApplyBuoyancy(FBO* velocity, FBO* temperature, FBO* density,
		FBO* dest)
{
	dest->bind();

	ApplyBuoyancyShdr->begin();

	ApplyBuoyancyShdr->setUniform1i("Velocity", 0);
	ApplyBuoyancyShdr->setUniform1i("Temperature", 1);
	ApplyBuoyancyShdr->setUniform1i("Density", 2);
	ApplyBuoyancyShdr->setUniform1f("AmbientTemperature", ambientTemperature);
	ApplyBuoyancyShdr->setUniform1f("TimeStep", timeStep);
	ApplyBuoyancyShdr->setUniform1f("Sigma", smokeBuoyancy);
	ApplyBuoyancyShdr->setUniform1f("Kappa", smokeWeight);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, velocity->getColorImg());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, temperature->getColorImg());
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, density->getColorImg());

	FullscreenQuad->drawInstanced(GL_TRIANGLES, gridSize.z, nullptr, 1.f);

	dest->unbind();
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::CreateObstacles(FBO* _fbo)
{
	VAO* lineVao = new VAO("position:3f", GL_DYNAMIC_DRAW);
	lineVao->initData(5);

#define T 0.9999f
	float positions[] = { -T, -T, 0, T, -T, 0, T, T, 0, -T, T, 0, -T, -T, 0 };
#undef T

	lineVao->upload(POSITION, positions, 5);

	VAO* circleVao = new VAO("position:3f", GL_DYNAMIC_DRAW);
	circleVao->initData(_fbo->getDepth() * 3);

	std::string obstFrag = STRINGIFY(
		out vec4 FragColor;
		void main() {
			FragColor = vec4(1.0, 0.0, 0.0, 1.0);
	});

	obstFrag = "// GLSLFluid3D obstacle shader\n" + shdr_Header + obstFrag;
	Shaders* obstShdr = shCol->addCheckShaderText("GLSLFluid3D_obstShdr",
			fluidVertex.c_str(), obstFrag.c_str());

	// ------------------------------------------------------------

	_fbo->bind();
	_fbo->clear();

	glDisable(GL_DEPTH_TEST);

	obstShdr->begin();

	for (unsigned int slice = 0; slice < _fbo->getDepth(); ++slice)
	{
		_fbo->set3DLayer(0, _fbo->getDepth() - 1 - slice);

		float z = _fbo->getDepth() / 2.0f;
		z = std::abs(slice - z) / z;
		float fraction = 1.f - std::sqrt(z);
		float radius = 0.5f * fraction;

		if (slice != 0 && slice != _fbo->getDepth() - 1)
			lineVao->draw(GL_LINE_STRIP, 0, 5, nullptr);

		if (slice == 0 || slice == _fbo->getDepth() - 1)
		{
			radius *= 100.f;

			const int slices = 64;
			float positions[slices * 3 * 3];
			float twopi = 8.f * std::atan(1.0f);
			float theta = 0.f;
			float dtheta = twopi / (float) (slices - 1);
			float* pPositions = &positions[0];

			for (int i = 0; i < slices; i++)
			{
				*pPositions++ = 0.f;
				*pPositions++ = 0.f;
				*pPositions++ = 0.f;

				*pPositions++ = radius * cos(theta);
				*pPositions++ = radius * sin(theta);
				theta += dtheta;
				*pPositions++ = 0.f;

				*pPositions++ = radius * cos(theta);
				*pPositions++ = radius * sin(theta);
				*pPositions++ = 0.f;
			}

			circleVao->upload(POSITION, positions, slices * 3);
			circleVao->draw(GL_TRIANGLES, 0, slices * 3, nullptr);
		}
	}

	_fbo->unbind();

	// obstShdr->remove();
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::initCommonShdr()
{
	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	fluidVertex = STRINGIFY(
		layout(location=0) in vec4 position;
		out int vInstance;
		void main() {
			gl_Position = position;
			vInstance = gl_InstanceID;
	});

	fluidVertex = "// GLSLFluid3D std vertex shader\n" + shdr_Header + fluidVertex;

	//------------------------------------------------------------------------------------

	fluidPickLayer = STRINGIFY(
		layout(triangles) in;
		layout(triangle_strip, max_vertices = 3) out;

		in int vInstance[3]; out float gLayer;

		void main() {
			gl_Layer = vInstance[0];
			gLayer = float(gl_Layer) + 0.5;

			gl_Position = gl_in[0].gl_Position;
			EmitVertex();

			gl_Position = gl_in[1].gl_Position;
			EmitVertex();

			gl_Position = gl_in[2].gl_Position;
			EmitVertex();

			EndPrimitive();
		});

	fluidPickLayer = "// GLSLFluid3D pickLayer geo shader\n" + shdr_Header + fluidPickLayer;
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::initRayCastShdr()
{
	std::string vert = STRINGIFY(
		layout(location = 0) in vec4 position;
		out vec4 vPosition;
		uniform mat4 ModelviewProjection;
		void main() {
			gl_Position = ModelviewProjection * position;
			vPosition = position;
		});

	vert = "// GLSLFluid3D raycast vertex shader\n" + shdr_Header + vert;


	std::string geom = STRINGIFY(
		layout(points) in;
		layout(triangle_strip, max_vertices = 24) out;

		in vec4 vPosition[1];

		uniform mat4 ModelviewProjection;

		vec4 objCube[8]; // Object space coordinate of cube corner
		vec4 ndcCube[8];// Normalized device coordinate of cube corner
		ivec4 faces[6];// Vertex indices of the cube faces

		void emit_vert(int vert) {
			gl_Position = ndcCube[vert];
			EmitVertex();
		}

		void emit_face(int face) {
			emit_vert(faces[face][1]);
			emit_vert(faces[face][0]);
			emit_vert(faces[face][3]);
			emit_vert(faces[face][2]);
			EndPrimitive();
		}

		void main() {
			faces[0] = ivec4(0,1,3,2);
			faces[1] = ivec4(5,4,6,7);
			faces[2] = ivec4(4,5,0,1);
			faces[3] = ivec4(3,2,7,6);
			faces[4] = ivec4(0,3,4,7);
			faces[5] = ivec4(2,1,6,5);

			vec4 P = vPosition[0];
			vec4 I = vec4(1.0,0,0,0);
			vec4 J = vec4(0,1.0,0,0);
			vec4 K = vec4(0,0,1.0,0);

			objCube[0] = P+K+I+J;
			objCube[1] = P+K+I-J;
			objCube[2] = P+K-I-J;
			objCube[3] = P+K-I+J;
			objCube[4] = P-K+I+J;
			objCube[5] = P-K+I-J;
			objCube[6] = P-K-I-J;
			objCube[7] = P-K-I+J;

			// Transform the corners of the box:
			for (int vert = 0; vert < 8; vert++)
				ndcCube[vert] = ModelviewProjection * objCube[vert];

			// Emit the six faces:
			for (int face = 0; face < 6; face++)
				emit_face(face);
		});

	geom = "// GLSLFluid3D raycast vertex shader\n" + shdr_Header + geom;



	std::string frag = STRINGIFY(
		out vec4 FragColor;

		uniform sampler3D Density;
		uniform sampler3D LightCache;
		uniform mat4 Modelview;
		uniform vec3 LightPosition = vec3(1.0, 1.0, 2.0);
		uniform vec3 LightIntensity = vec3(10.0);
		uniform vec3 RayOrigin;
		uniform vec3 Ambient = vec3(0.15, 0.15, 0.20);
		uniform vec2 WindowSize;

		uniform float Absorption = 10.0;
		uniform float FocalLength;
		uniform float StepSize;
		uniform int ViewSamples;

		const bool Jitter = false;

		float GetDensity(vec3 pos) {
			return texture(Density, pos).x;
		}

		struct Ray {
			vec3 Origin;
			vec3 Dir;
		};

		struct AABB {
			vec3 Min;
			vec3 Max;
		};

		bool IntersectBox(Ray r, AABB aabb, out float t0, out float t1)
		{
			vec3 invR = 1.0 / r.Dir;
			vec3 tbot = invR * (aabb.Min-r.Origin);
			vec3 ttop = invR * (aabb.Max-r.Origin);
			vec3 tmin = min(ttop, tbot);
			vec3 tmax = max(ttop, tbot);
			vec2 t = max(tmin.xx, tmin.yz);
			t0 = max(t.x, t.y);
			t = min(tmax.xx, tmax.yz);
			t1 = min(t.x, t.y);
			return t0 <= t1;
		}

		float randhash(uint seed, float b)
		{
			const float InverseMaxInt = 1.0 / 4294967295.0;
			uint i=(seed^12345391u)*2654435769u;
			i^=(i<<6u)^(i>>26u);
			i*=2654435769u;
			i+=(i<<5u)^(i>>12u);
			return float(b * i) * InverseMaxInt;
		}

		void main()
		{
			vec3 rayDirection;
			rayDirection.xy = 2.0 * gl_FragCoord.xy / WindowSize - 1.0;
			rayDirection.x /= WindowSize.y / WindowSize.x;
			rayDirection.z = -FocalLength;
			rayDirection = (vec4(rayDirection, 0.0) * Modelview).xyz;

			Ray eye = Ray( RayOrigin, normalize(rayDirection) );
			AABB aabb = AABB(vec3(-1.0), vec3(1.0));

			float tnear;
			float tfar;
			IntersectBox(eye, aabb, tnear, tfar);

			tnear = tnear * float( !(tnear < 0.0) );

			vec3 rayStart = eye.Origin + eye.Dir * tnear;
			vec3 rayStop = eye.Origin + eye.Dir * tfar;
			rayStart = 0.5 * (rayStart + 1.0);
			rayStop = 0.5 * (rayStop + 1.0);

			vec3 pos = rayStart;
			vec3 viewDir = normalize(rayStop-rayStart) * StepSize;
			float T = 1.0;
			vec3 Lo = Ambient;

			if (Jitter) {
				uint seed = uint(gl_FragCoord.x) * uint(gl_FragCoord.y);
				pos += viewDir * (-0.5 + randhash(seed, 1.0));
			}

			float remainingLength = distance(rayStop, rayStart);

			for (int i=0; i < ViewSamples && remainingLength > 0.0; ++i, pos += viewDir, remainingLength -= StepSize)
			{

				float density = GetDensity(pos);
				vec3 lightColor = vec3(1);

				if (pos.z < 0.1)
				{
					density = 10;
					lightColor = 3*Ambient;
				} else if (density <= 0.01) {
					continue;
				}

				T *= 1.0 - density * StepSize * Absorption;
				if (T <= 0.01) break;

				vec3 Li = lightColor * texture(LightCache, pos).xxx;
				Lo += Li * T * density * StepSize;
			}

			//Lo = 1-Lo;

			FragColor.rgb = Lo;
			FragColor.a = 1.0 - T;
//                                        FragColor.a = 0.8;
	});

	frag = "// GLSLFluid3D raycast frag shader\n" + shdr_Header + frag;

	RaycastShdr = shCol->addCheckShaderText("GLSLFluid3D_raycast", vert.c_str(),
			geom.c_str(), frag.c_str());
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::initLightShdr()
{
	std::string frag = STRINGIFY(
		in float gLayer;
		out float FragColor;

		uniform sampler3D Density;
		uniform vec3 LightPosition = vec3(1.0, 1.0, 2.0);
		uniform float LightIntensity = 10.0;
		uniform float Absorption = 10.0;
		uniform float LightStep;
		uniform int LightSamples;
		uniform vec3 InverseSize;

		float GetDensity(vec3 pos) {
			return texture(Density, pos).x;
		}

		void main() {
			vec3 pos = InverseSize * vec3(gl_FragCoord.xy, gLayer);
			vec3 lightDir = normalize(LightPosition-pos) * LightStep;
			float Tl = 1.0;
			vec3 lpos = pos + lightDir;

			for (int s = 0; s < LightSamples; ++s)
			{
				float ld = GetDensity(lpos);
				Tl *= 1.0 - Absorption * LightStep * ld;
				if (Tl <= 0.01) break;

				// Would be faster if this coniditional is replaced with a tighter loop
				float cond = float(lpos.x < 0.0) * float(lpos.y < 0.0) * float(lpos.z < 0.0)
						* float(lpos.x > 1.0) * float(lpos.y > 1.0) * float(lpos.z > 1.0);

				if (cond > 0.0) break;

				lpos += lightDir;
			}

			float Li = LightIntensity*Tl;
			FragColor = Li;
		});

	frag = "// GLSLFluid3D light frag shader\n" + shdr_Header + frag;

	LightShdr = shCol->addCheckShaderText("GLSLFluid3D_light",
			fluidVertex.c_str(), fluidPickLayer.c_str(), frag.c_str());
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::initBlurShdr()
{
	std::string frag = STRINGIFY(
		in float gLayer;
		out float FragColor;
		uniform sampler3D Density;
		uniform vec3 InverseSize;
		uniform float StepSize;
		uniform float DensityScale;

		float GetDensity(vec3 pos) {
			return texture(Density, pos).x * DensityScale;
		}

		// This implements a super stupid filter in 3-Space that takes 7 samples.
		// A three-pass seperable Gaussian would be way better.
		void main()
		{
			vec3 pos = InverseSize * vec3(gl_FragCoord.xy, gLayer);
			float e = StepSize;
			float z = e;
			float density = GetDensity(pos);
			density += GetDensity(pos + vec3(e,e,0));
			density += GetDensity(pos + vec3(-e,e,0));
			density += GetDensity(pos + vec3(e,-e,0));
			density += GetDensity(pos + vec3(-e,-e,0));
			density += GetDensity(pos + vec3(0,0,-z));
			density += GetDensity(pos + vec3(0,0,+z));
			density /= 7;
			FragColor = density;
		});

	frag = "// GLSLFluid3D light frag shader\n" + shdr_Header + frag;

	BlurShdr = shCol->addCheckShaderText("GLSLFluid3D_blur",
			fluidVertex.c_str(), fluidPickLayer.c_str(), frag.c_str());
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::initAdvect()
{
	std::string frag = STRINGIFY(
		out vec4 FragColor;

		uniform sampler3D VelocityTexture;
		uniform sampler3D SourceTexture;
		uniform sampler3D Obstacles;

		uniform vec3 InverseSize;
		uniform float TimeStep;
		uniform float Dissipation;

		in float gLayer;

		void main() {
			vec3 fragCoord = vec3(gl_FragCoord.xy, gLayer);
			float solid = texture(Obstacles, InverseSize * fragCoord).x;

			if (solid > 0) {
				FragColor = vec4(0);
				return;
			}

			vec3 u = texture(VelocityTexture, InverseSize * fragCoord).xyz;
			vec3 coord = InverseSize * (fragCoord - TimeStep * u);
			FragColor = Dissipation * texture(SourceTexture, coord);
	});


	frag = "// GLSLFluid3D Advect frag shader\n" + shdr_Header + frag;

	AdvectShdr = shCol->addCheckShaderText("GLSLFluid3D_advect",
			fluidVertex.c_str(), fluidPickLayer.c_str(), frag.c_str());
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::initJacobi()
{
	std::string frag = STRINGIFY(
		out vec4 FragColor;

		uniform sampler3D Pressure;
		uniform sampler3D Divergence;
		uniform sampler3D Obstacles;

		uniform float Alpha;
		uniform float InverseBeta;

		in float gLayer;

		void main() {
			ivec3 T = ivec3(gl_FragCoord.xy, gLayer);

			// Find neighboring pressure:
			vec4 pN = texelFetchOffset(Pressure, T, 0, ivec3(0, 1, 0));
			vec4 pS = texelFetchOffset(Pressure, T, 0, ivec3(0, -1, 0));
			vec4 pE = texelFetchOffset(Pressure, T, 0, ivec3(1, 0, 0));
			vec4 pW = texelFetchOffset(Pressure, T, 0, ivec3(-1, 0, 0));
			vec4 pU = texelFetchOffset(Pressure, T, 0, ivec3(0, 0, 1));
			vec4 pD = texelFetchOffset(Pressure, T, 0, ivec3(0, 0, -1));
			vec4 pC = texelFetch(Pressure, T, 0);

			// Find neighboring obstacles:
			vec3 oN = texelFetchOffset(Obstacles, T, 0, ivec3(0, 1, 0)).xyz;
			vec3 oS = texelFetchOffset(Obstacles, T, 0, ivec3(0, -1, 0)).xyz;
			vec3 oE = texelFetchOffset(Obstacles, T, 0, ivec3(1, 0, 0)).xyz;
			vec3 oW = texelFetchOffset(Obstacles, T, 0, ivec3(-1, 0, 0)).xyz;
			vec3 oU = texelFetchOffset(Obstacles, T, 0, ivec3(0, 0, 1)).xyz;
			vec3 oD = texelFetchOffset(Obstacles, T, 0, ivec3(0, 0, -1)).xyz;

			// Use center pressure for solid cells:
			pN = mix(pN, pC, sign(oN.x));
			pS = mix(pS, pC, sign(oS.x));
			pE = mix(pE, pC, sign(oE.x));
			pW = mix(pW, pC, sign(oW.x));
			pU = mix(pU, pC, sign(oU.x));
			pD = mix(pD, pC, sign(oD.x));

			vec4 bC = texelFetch(Divergence, T, 0);
			FragColor = (pW + pE + pS + pN + pU + pD + Alpha * bC) * InverseBeta;
		});

	frag = "// GLSLFluid3D Jacobi frag shader\n" + shdr_Header + frag;

	JacobiShdr = shCol->addCheckShaderText("GLSLFluid3D_jacobi",
			fluidVertex.c_str(), fluidPickLayer.c_str(), frag.c_str());
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::initSubtractGrad()
{
	std::string frag = STRINGIFY(
		out vec3 FragColor;

		uniform sampler3D Velocity;
		uniform sampler3D Pressure;
		uniform sampler3D Obstacles;
		uniform float GradientScale;

		in float gLayer;

		void main() {
			ivec3 T = ivec3(gl_FragCoord.xy, gLayer);

			vec3 oC = texelFetch(Obstacles, T, 0).xyz;
			if (oC.x > 0) {
				FragColor = oC.yzx;
				return;
			}

			// Find neighboring pressure:
			float pN = texelFetchOffset(Pressure, T, 0, ivec3(0, 1, 0)).r;
			float pS = texelFetchOffset(Pressure, T, 0, ivec3(0, -1, 0)).r;
			float pE = texelFetchOffset(Pressure, T, 0, ivec3(1, 0, 0)).r;
			float pW = texelFetchOffset(Pressure, T, 0, ivec3(-1, 0, 0)).r;
			float pU = texelFetchOffset(Pressure, T, 0, ivec3(0, 0, 1)).r;
			float pD = texelFetchOffset(Pressure, T, 0, ivec3(0, 0, -1)).r;
			float pC = texelFetch(Pressure, T, 0).r;

			// Find neighboring obstacles:
			vec3 oN = texelFetchOffset(Obstacles, T, 0, ivec3(0, 1, 0)).xyz;
			vec3 oS = texelFetchOffset(Obstacles, T, 0, ivec3(0, -1, 0)).xyz;
			vec3 oE = texelFetchOffset(Obstacles, T, 0, ivec3(1, 0, 0)).xyz;
			vec3 oW = texelFetchOffset(Obstacles, T, 0, ivec3(-1, 0, 0)).xyz;
			vec3 oU = texelFetchOffset(Obstacles, T, 0, ivec3(0, 0, 1)).xyz;
			vec3 oD = texelFetchOffset(Obstacles, T, 0, ivec3(0, 0, -1)).xyz;

			// Use center pressure for solid cells:
			vec3 obstV = vec3(0);
			vec3 vMask = vec3(1);

			if (oN.x > 0) {
				pN = pC;
				obstV.y = oN.z;
				vMask.y = 0;
			}

			if (oS.x > 0) {
				pS = pC;
				obstV.y = oS.z;
				vMask.y = 0;
			}

			if (oE.x > 0) {
				pE = pC;
				obstV.x = oE.y;
				vMask.x = 0;
			}

			if (oW.x > 0) {
				pW = pC;
				obstV.x = oW.y;
				vMask.x = 0;
			}

			if (oU.x > 0) {
				pU = pC;
				obstV.z = oU.x;
				vMask.z = 0;
			}

			if (oD.x > 0) {
				pD = pC;
				obstV.z = oD.x;
				vMask.z = 0;
			}

			// Enforce the free-slip boundary condition:
			vec3 oldV = texelFetch(Velocity, T, 0).xyz;
			vec3 grad = vec3(pE - pW, pN - pS, pU - pD) * GradientScale;
			vec3 newV = oldV - grad;
			FragColor = (vMask * newV) + obstV;
		});

	frag = "// GLSLFluid3D Jacobi SubtractGradient shader\n" + shdr_Header
			+ frag;

	SubtractGradientShdr = shCol->addCheckShaderText("GLSLFluid3D_subtrGrad",
			fluidVertex.c_str(), fluidPickLayer.c_str(), frag.c_str());
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::initComputeDiv()
{
	std::string frag =
			STRINGIFY(
					out float FragColor;

					uniform sampler3D Velocity; uniform sampler3D Obstacles; uniform float HalfInverseCellSize;

					in float gLayer;

					void main() { ivec3 T = ivec3(gl_FragCoord.xy, gLayer);

					// Find neighboring velocities:
					vec3 vN = texelFetchOffset(Velocity, T, 0, ivec3(0, 1, 0)).xyz; vec3 vS = texelFetchOffset(Velocity, T, 0, ivec3(0, -1, 0)).xyz; vec3 vE = texelFetchOffset(Velocity, T, 0, ivec3(1, 0, 0)).xyz; vec3 vW = texelFetchOffset(Velocity, T, 0, ivec3(-1, 0, 0)).xyz; vec3 vU = texelFetchOffset(Velocity, T, 0, ivec3(0, 0, 1)).xyz; vec3 vD = texelFetchOffset(Velocity, T, 0, ivec3(0, 0, -1)).xyz;

					// Find neighboring obstacles:
					vec3 oN = texelFetchOffset(Obstacles, T, 0, ivec3(0, 1, 0)).xyz; vec3 oS = texelFetchOffset(Obstacles, T, 0, ivec3(0, -1, 0)).xyz; vec3 oE = texelFetchOffset(Obstacles, T, 0, ivec3(1, 0, 0)).xyz; vec3 oW = texelFetchOffset(Obstacles, T, 0, ivec3(-1, 0, 0)).xyz; vec3 oU = texelFetchOffset(Obstacles, T, 0, ivec3(0, 0, 1)).xyz; vec3 oD = texelFetchOffset(Obstacles, T, 0, ivec3(0, 0, -1)).xyz;

					// Use obstacle velocities for solid cells:
					if (oN.x > 0) vN = oN.yzx; if (oS.x > 0) vS = oS.yzx; if (oE.x > 0) vE = oE.yzx; if (oW.x > 0) vW = oW.yzx; if (oU.x > 0) vU = oU.yzx; if (oD.x > 0) vD = oD.yzx;

					FragColor = HalfInverseCellSize * (vE.x - vW.x + vN.y - vS.y + vU.z - vD.z); });

	frag = "// GLSLFluid3D Jacobi ComputeDivergence shader\n" + shdr_Header
			+ frag;

	ComputeDivergenceShdr = shCol->addCheckShaderText("GLSLFluid3D_compDiv",
			fluidVertex.c_str(), fluidPickLayer.c_str(), frag.c_str());
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::initApplyImp()
{
	std::string frag = STRINGIFY(
		out vec4 FragColor;

		uniform vec3 Point;
		uniform float Radius;
		uniform vec3 FillColor;

		in float gLayer;

		void main() {
			float d = distance(Point, vec3(gl_FragCoord.xy, gLayer));
			float a = (Radius - d) * 0.5;
			a = min(a, 1.0);
			FragColor = vec4(FillColor, a) * float(d < Radius);
		});

	frag = "// GLSLFluid3D Jacobi ApplyImpulse shader\n" + shdr_Header + frag;

	ApplyImpulseShdr = shCol->addCheckShaderText("GLSLFluid3D_applyImp",
			fluidVertex.c_str(), fluidPickLayer.c_str(), frag.c_str());
}

//------------------------------------------------------------------------------------

void GLSLFluid3D::initApplyBuoyancy()
{
	std::string frag = STRINGIFY(
		out vec3 FragColor;
		uniform sampler3D Velocity;
		uniform sampler3D Temperature;
		uniform sampler3D Density;
		uniform float AmbientTemperature;
		uniform float TimeStep;
		uniform float Sigma;
		uniform float Kappa;

		in float gLayer;

		void main()
		{
			ivec3 TC = ivec3(gl_FragCoord.xy, gLayer);
			float T = texelFetch(Temperature, TC, 0).r;
			vec3 V = texelFetch(Velocity, TC, 0).xyz;

			FragColor = V;

			if (T > AmbientTemperature)
			{
				float D = texelFetch(Density, TC, 0).x;
				FragColor += (TimeStep * (T - AmbientTemperature) * Sigma - D * Kappa ) * vec3(0, -1, 0);
			}
		});

	frag = "// GLSLFluid3D Jacobi ApplyBuoyancy shader\n" + shdr_Header + frag;

	ApplyBuoyancyShdr = shCol->addCheckShaderText("GLSLFluid3D_applBuoyancy",
			fluidVertex.c_str(), fluidPickLayer.c_str(), frag.c_str());
}

//------------------------------------------------------------------------------------

int GLSLFluid3D::getWidth()
{
	return gridSize.x;
}

//------------------------------------------------------------------------------------

int GLSLFluid3D::getHeight()
{
	return gridSize.y;
}

//------------------------------------------------------------------------------------

int GLSLFluid3D::getDepth()
{
	return gridSize.z;
}

/*

 //------------------------------------------------------------------------------------
 // input in pixeln, links unten ist 0|0
 
 void GLSLFluid3D::addTemporalForce(glm::vec2 _pos, glm::vec2 _vel, glm::vec4 _col, float _rad, float _temp, float _den)
 {
 punctualForce f;
 
 f.pos = _pos * scale;
 f.vel = _vel;
 f.color = _col;
 f.rad = _rad;
 f.temp = _temp;
 f.den = _den;
 
 temporalForces.push_back(f);
 }
 
 //------------------------------------------------------------------------------------
 
 void GLSLFluid3D::addTemporalVelocity(glm::vec2 _pos, glm::vec2 _vel, float _rad, float _temp, float _den)
 {
 punctualForce f;
 
 f.pos = _pos * scale;
 f.vel = _vel;
 f.rad = _rad;
 f.temp = _temp;
 f.den = _den;
 
 temporalVelocity.push_back(f);
 }
 
 //------------------------------------------------------------------------------------
 
 void GLSLFluid3D::addConstantForce(glm::vec2 _pos, glm::vec2 _vel,  glm::vec4 _col, float _rad, float _temp, float _den)
 {
 punctualForce f;
 
 f.pos = _pos * scale;
 f.vel = _vel;
 f.color = _col;
 f.rad = _rad;
 f.temp = _temp;
 f.den = _den;
 
 constantForces.push_back(f);
 }
 
 //------------------------------------------------------------------------------------
 
 void GLSLFluid3D::setObstacles(GLint texNr)
 {
 obstaclesFbo->bind();
 obstaclesFbo->clear();
 
 stdTexShader->begin();
 stdTexShader->setIdentglm::mat4fv("m_pvm");
 stdTexShader->setUniform1i("tex", 0);
 
 glActiveTexture(GL_TEXTURE0);
 glBindTexture(GL_TEXTURE_2D, texNr);
 
 quad->draw();
 stdTexShader->end();
 
 obstaclesFbo->unbind();
 }
 
 //------------------------------------------------------------------------------------
 
 void GLSLFluid3D::setObstacles(TextureManager* _tex)
 {
 stdTexShader->begin();
 stdTexShader->setIdentglm::mat4fv("m_pvm");
 obstaclesFbo->bind();
 _tex->bind();
 quad->draw();
 stdTexShader->end();
 obstaclesFbo->unbind();
 }
 
 //--- add Color with a gl Texture ---------------------------------------------------------------------------------
 
 void GLSLFluid3D::addColor(GLint _tex, glm::vec3 _multCol, float _pct, bool _asBW)
 {
 applyImpMultCol = _multCol;
 colorAddTex = _tex;
 colorAddPct = _pct;
 colorAddPctAsBW = _asBW;
 }
 
 //--- add Velocity with a gl Texture ---------------------------------------------------------------------------------
 
 void GLSLFluid3D::addVelocity(GLint _tex, float _pct)
 {
 velocityAddTex = _tex;
 velocityAddPct = _pct;
 }
 
 //------------------------------------------------------------------------------------
 
 void GLSLFluid3D::clear(float _alpha)
 {
 pingPong->clear();
 velocityBuffer->clear();
 temperatureBuffer->clear();
 pressureBuffer->clear();
 
 obstaclesFbo->bind();
 obstaclesFbo->clear();
 divergenceFbo->bind();
 divergenceFbo->clear();
 
 temperatureBuffer->clear(ambientTemperature);
 }    
 
 //------------------------------------------------------------------------------------
 
 void GLSLFluid3D::drawVelocity()
 {
 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 
 stdTexShader->begin();
 stdTexShader->setIdentglm::mat4fv("m_pvm");
 stdTexShader->setUniform1i("tex", 0);
 stdTexShader->setUniform2f("tScale", 1.f, 1.f);
 
 glActiveTexture(GL_TEXTURE0);
 glBindTexture(GL_TEXTURE_2D, velocityBuffer->getSrcTexId());
 quad->draw();
 
 stdTexShader->end();
 }
 
 //------------------------------------------------------------------------------------
 
 // With begin(int) and end(int) the textures allocated can be filled with data
 void GLSLFluid3D::begin(int _texNum )
 {
 if ((_texNum < nTextures) && ( _texNum >= 0))
 {
 textures[_texNum]->bind();
 textures[_texNum]->clear();
 }
 }
 
 //------------------------------------------------------------------------------------
 
 void GLSLFluid3D::end(int _texNum)
 {
 if ((_texNum < nTextures) && ( _texNum >= 0))
 {
 textures[_texNum]->unbind();
 }
 }
 
 //------------------------------------------------------------------------------------
 
 GLint GLSLFluid3D::getResTex()
 {
 GLint out = 0;
 out = pingPong->getSrcTexId();
 return out;
 }
 
 //------------------------------------------------------------------------------------
 
 GLint GLSLFluid3D::getVelocityTex()
 {
 GLint out = 0;
 out = velocityBuffer->getSrcTexId();
 return out;
 }

 //------------------------------------------------------------------------------------
 
 int GLSLFluid3D::getVelTexWidth()
 {
 return gridSize.x;
 }
 
 //------------------------------------------------------------------------------------
 
 int GLSLFluid3D::getVelTexHeight()
 {
 return gridHeight;
 }
 
 //------------------------------------------------------------------------------------
 
 void GLSLFluid3D::setVelTexThresh(float _velTexThresh)
 {
 applyVelTexThresh = _velTexThresh;
 }
 
 //------------------------------------------------------------------------------------
 
 void GLSLFluid3D::setSmokeBuoyancy(float _val)
 {
 smokeBuoyancy = _val;
 }
 
 //------------------------------------------------------------------------------------
 
 void GLSLFluid3D::setVelTexRadius(float _val)
 {
 applyVelTextureRadius = _val;
 }
 
 //------------------------------------------------------------------------------------
 
 void GLSLFluid3D::cleanUp()
 {
 }
 */
//------------------------------------------------------------------------------------
GLSLFluid3D::~GLSLFluid3D()
{
	ApplyImpulseShdr->remove();
	ApplyBuoyancyShdr->remove();
	AdvectShdr->remove();
	BlurShdr->remove();
	ComputeDivergenceShdr->remove();
	JacobiShdr->remove();
	LightShdr->remove();
	RaycastShdr->remove();
	SubtractGradientShdr->remove();

}
}
