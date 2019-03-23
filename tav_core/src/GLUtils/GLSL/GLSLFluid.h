//
//  GLSLFluid.h
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__GLSLFluid__
#define __Tav_App__GLSLFluid__

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
class GLSLFluid
{
public:
	typedef struct
	{
		glm::vec4 color;
		glm::vec2 pos;
		glm::vec2 vel;
		float rad;
		float temp;
		float den;
	} punctualForce;

	GLSLFluid(bool _bObstacles, ShaderCollector* _shCol);
	~GLSLFluid();

	void allocate(int _width, int _height, float _scale = 0.5);
	void setGravity(glm::vec2 _force)
	{
		gForce = _force;
	}
	;
	void addTemporalForce(glm::vec2 _pos, glm::vec2 _vel, glm::vec4 _col,
			float _rad = 1.0f, float _temp = 10.f, float _den = 1.f);
	void addTemporalVelocity(glm::vec2 _pos, glm::vec2 _vel, float _rad = 1.0f,
			float _temp = 10.f, float _den = 1.f);
	void addConstantForce(glm::vec2 _pos, glm::vec2 _vel, glm::vec4 _col,
			float _rad = 1.0f, float _temp = 10.f, float _den = 1.f);
	void setObstacles(TextureManager* _tex);
	void setObstacles(GLint texNr);
	void addColor(GLint _tex, glm::vec3 _multCol, float _pct = 1.0, bool _asBW =
			false);
	void addVelocity(GLint _tex, float _pct = 1.0);
	void clear(float _alpha = 1.f);
	void update();
	void draw();
	void drawVelocity();
	void advect(PingPongFbo* _buffer, float _dissipation);
	void jacobi();
	void subtractGradient();
	void computeDivergence();
	void applyImpulse(PingPongFbo* _buffer, GLint _baseTex, glm::vec3& _multVec,
			float _pct = 1.0, bool _isVel = false, bool _asBW = false);
	void applyImpulse(PingPongFbo* _buffer, glm::vec2 _force, glm::vec4 _value,
			float _radio = 3.f, bool _isVel = false);
	void applyBuoyancy();
	void begin(int _texNum = 0);
	void end(int _texNum = 0);
	GLint getResTex();
	GLint getVelocityTex();
	int getWidth();
	int getHeight();
	int getVelTexWidth();
	int getVelTexHeight();
	void setVelTexThresh(float _velTexThresh);
	void setAmbientTemp(float _val);
	void setUseBuoyancy(bool _val);
	void setSmokeBuoyancy(float _val);
	void setSmokeWeight(float _val);
	void setTimeStep(float _val);
	void setVelTexRadius(float _val);
	void initShaders(bool obstacles);
	void cleanUp();

	bool useBuoyancy = true;
	float dissipation;
	float velocityDissipation;
	float temperatureDissipation;
	float pressureDissipation;

	float applyVelTextureRadius;
	float applyColTexAlphaScale = 1.f;
	float applyVelTexThresh = 0.125f;

	FBO* obstaclesFbo = NULL;
	FBO* divergenceFbo = NULL;

	PingPongFbo* pingPong = NULL;
	PingPongFbo* velocityBuffer = NULL;
	PingPongFbo* temperatureBuffer = NULL;
	PingPongFbo* pressureBuffer = NULL;

private:
	Shaders* jacobiShader = NULL;
	Shaders* subtractGradientShader = NULL;
	Shaders* computeDivergenceShader = NULL;
	Shaders* applyImpulseShader = NULL;
	Shaders* applyTextureShader = NULL;
	Shaders* applyBuoyancyShader = NULL;
	Shaders* stdTexShader = NULL;

	Quad* quad;

	std::vector<punctualForce> constantForces;
	std::vector<punctualForce> temporalForces;
	std::vector<punctualForce> temporalVelocity;

	glm::vec2 gForce;
	glm::vec3 applyImpMultCol;
	glm::vec3 whiteCol;

	float smokeBuoyancy;
	float smokeWeight;
	float gradientScale;
	float ambientTemperature;

	float gridWidth, gridHeight;
	float timeStep;
	float cellSize;
	float scale;

	int numJacobiIterations;

	bool bObstacles = false;
	bool colorAddPctAsBW = false;

	float colorAddPct, velocityAddPct;
	int colorGlFormat;
	int width, height;

	GLint velocityAddTex;
	GLint colorAddTex;

	FBO** textures;
	int nTextures = 0;
	int internalFormat;
	Shaders* shader;
	ShaderCollector* shCol;
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

	glm::mat4 camMat;
};
}

#endif /* defined(__Tav_App__GLSLFluid__) */
