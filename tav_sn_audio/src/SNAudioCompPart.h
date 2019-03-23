//
// SNAudioCompPart.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include "Shaders/ShaderCollector.h"
#include <PAudio.h>
#include "Communication/OSC/OSCData.h"
#include <SceneNode.h>
#include "Shaders/ShaderBuffer.h"
#include <Median.h>

namespace tav
{

class SNAudioCompPart: public SceneNode
{
public:
	struct ShaderParams
	{
		glm::mat4 ModelView;
		glm::mat4 ModelViewProjection;
		glm::mat4 ProjectionMatrix;
		glm::vec4 attractor;
		uint numParticles;
		float spriteSize;
		float damping;
		float initAmt;
		float noiseFreq;
		float noiseStrength;

		float time;
		float persistence;
		float NOISE_POSITION_SCALE;
		float NOISE_SCALE;
		float NOISE_TIME_SCALE;

		ShaderParams() :
				spriteSize(0.015f), attractor(0.0f, 0.0f, 0.0f, 0.0f), damping(
						0.95f), noiseFreq(10.0f), initAmt(1.f), noiseStrength(
						0.001f)
		{
		}
	};

	struct EmitParams
	{
		glm::vec4 ipos;
		glm::vec4 ivel;
		glm::vec4 iacc;
		glm::vec4 prevPos;
		glm::vec4 orientation;
		float radius;
		float velocityScale;
		float averageLifespan;
		float lifespanVariation;
		float emissionAngle;
		float averageVelocity;
		float velocityVariation;
		float useEmitterVelocity;
//		int id;
//		int count;
		EmitParams() :
				velocityScale(1.f), averageLifespan(120.f), lifespanVariation(
						0.f), emissionAngle(0.0f), radius(1.f), useEmitterVelocity(
						1.f)
		{
		}
	};

	SNAudioCompPart(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNAudioCompPart();

	void draw(void);
	void initDrawShdr();
	std::string initCurlShdr();
	std::string initEmitShdr();
	std::string getEmitterUtilityCode();
	std::string getEmitterCode();

	void initTfoShdr(TFO* _tfo);
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void reset(glm::vec3 size);
	void emit(double time, double _dt, unsigned int chanNr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);

private:
	GLuint createShaderPipelineProgram(GLuint target, GLuint prog,
			const char* src);
	std::string getNoiseShaderFunctions();

	PAudio* pa;
	OSCData* osc;
	ShaderParams mShaderParams;
	EmitParams mEmitParams;

	ShaderCollector* shCol;
	Shaders* mRenderProg;
	Shaders* mTfoProg;

	TextureManager* tex0;

	VAO* testVAO;

	const static int work_group_size = 256;
	const static int mNumParticles = work_group_size * 80;

	GLuint eUBO;
	GLuint mUBO;
	GLuint mVBO;

	GLint uboSize;
	GLuint uboIndex;

	bool mReset;
	bool inited = false;
	bool useTfo = false;

	float alpha = 0.f;
	float propo;
	float globalScale;
	float spriteSize;
	float initAmt;

	int oldReset = 0;
	int m_noiseSize;
	int emitCount = 0;

	double lastTime = 0.0;
	double lastPartUpdt = 0.0;

	glm::mat4 view;
	glm::mat4 proj;

	ShaderBuffer<glm::vec4>** m_pos;
	ShaderBuffer<glm::vec4>** m_init_pos;
	ShaderBuffer<glm::vec4>** m_acc;
	ShaderBuffer<glm::vec4>** m_vel;
	ShaderBuffer<uint32_t>* m_indices;
	ShaderBuffer<float>** m_lifespans;
	ShaderBuffer<int>** m_emitterIds;

	GLuint m_programCurlNoise;
	GLuint m_curlNoiseProg;

	GLuint m_programUpdate;
	GLuint m_updateProg;

	GLuint m_programEmit;
	GLuint m_emitProg;

	const char* m_shaderPrefix;
	// int work_group_size = 128;

	glm::vec3* prevPos;
	glm::vec3* prevVel;
	double* prevTime;

	glm::vec4* chanCols;

	Median<float>** pitchMed;
};

}
