//
//  GLSLFluid3D.h
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__GLSLFluid3D__
#define __Tav_App__GLSLFluid3D__

#pragma once

#include <vector>
#include <cstring>
#include <stdio.h>

#include "GeoPrimitives/Quad.h"
#include "GLUtils/PingPongFbo.h"
#include "Shaders/ShaderCollector.h"
#include "GLUtils/TextureManager.h"
#include "GLUtils/glm_utils.h"

#define STRINGIFY(A) #A

namespace tav
{
class GLSLFluid3D
{
public:
	enum AttributeSlot
	{
		SlotPosition, SlotTexCoord,
	};

	struct TexturePod
	{
		GLuint Handle;
		GLsizei Width;
		GLsizei Height;
	};

	struct
	{
		PingPongFbo* Velocity;
		PingPongFbo* Density;
		PingPongFbo* Pressure;
		PingPongFbo* Temperature;
	} Slabs;

	struct
	{
		FBO* Divergence;
		FBO* Obstacles;
		FBO* LightCache;
		FBO* BlurredDensity;
	} Surfaces;

	struct
	{
		glm::mat4 Projection;
		glm::mat4 Modelview;
		glm::mat4 View;
		glm::mat4 ModelviewProjection;
	} Matrices;

	GLSLFluid3D(ShaderCollector* _shCol);
	~GLSLFluid3D();

	void update(double time);
	void draw();

	void Advect(FBO* velocity, FBO* source, FBO* obstacles, FBO* dest,
			float dissipation);
	void Jacobi(FBO* pressure, FBO* divergence, FBO* obstacles, FBO* dest);
	void SubtractGradient(FBO* velocity, FBO* pressure, FBO* obstacles,
			FBO* dest);
	void ComputeDivergence(FBO* velocity, FBO* obstacles, FBO* dest);
	void ApplyImpulse(FBO* dest, glm::vec3 position, float value);
	void ApplyBuoyancy(FBO* velocity, FBO* temperature, FBO* density,
			FBO* dest);

	void CreateObstacles(FBO* _fbo);
	void initCommonShdr();
	void initRayCastShdr();
	void initLightShdr();
	void initBlurShdr();
	void initAdvect();
	void initJacobi();
	void initSubtractGrad();
	void initComputeDiv();
	void initApplyImp();
	void initApplyBuoyancy();

	int getWidth();
	int getHeight();
	int getDepth();

	/*
	 void setGravity(glm::vec2 _force){ gForce = _force; };
	 void addTemporalForce(glm::vec2 _pos, glm::vec2 _vel, glm::vec4 _col, float _rad = 1.0f,
	 float _temp = 10.f, float _den = 1.f);
	 void addTemporalVelocity(glm::vec2 _pos, glm::vec2 _vel, float _rad = 1.0f,
	 float _temp = 10.f, float _den = 1.f);
	 void addConstantForce(glm::vec2 _pos, glm::vec2 _vel,  glm::vec4 _col, float _rad = 1.0f,
	 float _temp = 10.f, float _den = 1.f);
	 void setObstacles(TextureManager* _tex);
	 void setObstacles(GLint texNr);
	 void addColor(GLint _tex, glm::vec3 _multCol, float _pct = 1.0, bool _asBW = false);
	 void addVelocity(GLint _tex, float _pct = 1.0);
	 void clear(float _alpha = 1.f);
	 void drawVelocity();
	 void begin(int _texNum = 0);
	 void end(int _texNum = 0);
	 GLint getResTex();
	 GLint getVelocityTex();
	 int getVelTexWidth();
	 int getVelTexHeight();
	 void setVelTexThresh(float _velTexThresh);
	 void setSmokeBuoyancy(float _val);
	 void setVelTexRadius(float _val);
	 void cleanUp();
	 
	 float           applyVelTextureRadius;
	 float           applyColTexAlphaScale = 1.f;
	 float           applyVelTexThresh = 0.125f;
	 */

private:
	ShaderCollector* shCol;
	Shaders* stdTexShader = NULL;
	Shaders* ApplyImpulseShdr;
	Shaders* ApplyBuoyancyShdr;
	Shaders* AdvectShdr;
	Shaders* BlurShdr;
	Shaders* ComputeDivergenceShdr;
	Shaders* JacobiShdr;
	Shaders* LightShdr;
	Shaders* RaycastShdr;
	Shaders* SubtractGradientShdr;

	VAO* CubeCenter;
	Quad* FullscreenQuad;

	std::string shdr_Header;
	std::string fluidVertex;
	std::string fluidPickLayer;

	glm::ivec3 gridSize;
	glm::vec3 eyePosition;

	glm::mat4 camMat;

	bool useBuoyancy = true;

	int viewSamples;
	int lightSamples;
	int numJacobiIterations;

	float fieldOfView;
	float defaultThetaX;
	float defaultThetaY;
	float thetaX;
	float thetaY;
	float fips;

	float cellSize;
	float splatRadius;
	float ambientTemperature;
	float impulseTemperature;
	float impulseDensity;
	float timeStep;
	float smokeBuoyancy;
	float smokeWeight;
	float gradientScale;
	float densityDissipation;
	float dissipation;
	float velocityDissipation;
	float temperatureDissipation;
	float pressureDissipation;
	glm::vec3 impulsePosition;

	/*
	 Shaders*        jacobiShader = NULL;
	 Shaders*        subtractGradientShader = NULL;
	 Shaders*        computeDivergenceShader = NULL;
	 Shaders*        applyImpulseShader = NULL;
	 Shaders*        applyTextureShader = NULL;
	 Shaders*        applyBuoyancyShader = NULL;
	 
	 Quad*           quad;
	 
	 std::vector<punctualForce> constantForces;
	 std::vector<punctualForce> temporalForces;
	 std::vector<punctualForce> temporalVelocity;
	 
	 glm::vec2       gForce;
	 glm::vec3       applyImpMultCol;
	 glm::vec3       whiteCol;
	 
	 float           scale;
	 
	 int             numJacobiIterations;
	 
	 bool            bObstacles = false;
	 bool            colorAddPctAsBW = false;
	 
	 float           colorAddPct, velocityAddPct;
	 int             colorGlFormat;
	 int             width, height;
	 
	 GLint           velocityAddTex;
	 GLint           colorAddTex;
	 
	 
	 FBO**           textures;
	 int             nTextures = 0;
	 int             internalFormat;
	 Shaders*        shader;
	 std::string vs;
	 std::string fragmentShader;
	 std::string fragmentShaderNoObst;
	 std::string fragmentJacobiShader;
	 std::string fragmentJacobiShaderNoObst;
	 std::string fragSubtractGradientShader;
	 std::string fragSubtractGradientShaderNoObst;
	 std::string fragmentComputeDivergenceShader;
	 std::string fragmentComputeDivergenceShaderNoObst;
	 std::string fragmentApplyTextureShader;
	 std::string fragmentApplyImpulseShader;
	 std::string fragmentApplyBuoyancyShader;
	 */
};
}

#endif /* defined(__Tav_App__GLSLFluid3D__) */
