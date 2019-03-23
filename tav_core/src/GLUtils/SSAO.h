//
// SSAO.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include "SceneNode.h"
#include "GLUtils/TextureManager.h"
#include "GLUtils/Vertex.h"
#include "GLUtils/Geometry.h"
#include "GLUtils/CameraControl.h"
#include "GLUtils/MersenneTwister.h"
#include "math_utils.h"
#include "Shaders/ShaderBuffer.h"
#include "GLUtils/Noise3DTexGen.h"
#include "GeoPrimitives/Quad.h"

#include <glm/ext.hpp>
#include <glm/gtx/color_space.hpp>

// optimizes blur, by storing depth along with ssao calculation
// avoids accessing two different textures
#define USE_AO_SPECIALBLUR 1

// optimizes the cache-aware technique by rendering all temporary layers at once
// instead of individually
#define AO_LAYERED_OFF    0
#define AO_LAYERED_IMAGE  1
#define AO_LAYERED_GS     2
#define AO_RANDOMTEX_SIZE 4
#define USE_AO_LAYERED_SINGLEPASS   AO_LAYERED_GS

#define VERTEX_POS    0
#define VERTEX_NORMAL 1
#define VERTEX_COLOR  2
#define UBO_SCENE     0
#define NV_BUFFER_OFFSET(i) ((char *)NULL + (i))

namespace tav
{

#ifndef RSC_GLUINT
#define RSC_GLUINT

class ResourceGLuint
{
public:
	GLuint m_value;

	ResourceGLuint() :
			m_value(0)
	{
	}
	ResourceGLuint(GLuint b) :
			m_value(b)
	{
	}
	operator GLuint() const
	{
		return m_value;
	}
	operator GLuint&()
	{
		return m_value;
	}
	ResourceGLuint& operator=(GLuint b)
	{
		m_value = b;
		return *this;
	}
};

#endif

struct Projection
{
	float nearplane;
	float farplane;
	float fov;
	float orthoheight;
	bool ortho;
	glm::mat4 matrix;

	Projection() :
			nearplane(0.1f), farplane(100.0f), fov((40.f)), orthoheight(1.0f), ortho(
					false)
	{
	}

	void update(int width, int height)
	{
		float aspect = float(width) / float(height);
		if (ortho)
		{
			matrix = glm::ortho(-orthoheight * 0.5f * aspect,
					orthoheight * 0.5f * aspect, -orthoheight * 0.5f,
					orthoheight * 0.5f, nearplane, farplane);
		}
		else
		{
			matrix = glm::perspective(fov, aspect, nearplane, farplane);
		}
	}
};

class SSAO
{
public:
	static const int NUM_MRT = 8;
	static const int HBAO_RANDOM_SIZE = AO_RANDOMTEX_SIZE;
	static const int HBAO_RANDOM_ELEMENTS = HBAO_RANDOM_SIZE * HBAO_RANDOM_SIZE;
	static const int MAX_SAMPLES = 8;

	enum AlgorithmType
	{
		ALGORITHM_NONE,
		ALGORITHM_HBAO_CACHEAWARE,
		ALGORITHM_HBAO_CLASSIC,
		NUM_ALGORITHMS,
	};

	struct
	{
		ResourceGLuint scene, depthlinear, viewnormal, hbao_calc,
				hbao2_deinterleave, hbao2_calc;
	} fbos;

	struct
	{
		ResourceGLuint scene_color, scene_depthstencil, scene_depthlinear,
				scene_viewnormal, hbao_result, hbao_blur, hbao_random,
				hbao_randomview[MAX_SAMPLES], hbao2_deptharray,
				hbao2_depthview[HBAO_RANDOM_ELEMENTS], hbao2_resultarray;
	} textures;

	struct SsaoPar
	{
		SsaoPar() :
				radius(2.f), bias(0.1f)
		{
		}

		float bias;
		float radius;
	};

	struct SceneData
	{
		glm::mat4 viewProjMatrix;
		glm::mat4 viewMatrix;
		glm::mat4 viewMatrixIT;
		glm::uvec2 viewport;
		glm::uvec2 _pad;
	};

	// corresponds to the uniform block -> later more elegantglm::
	struct HBAOData
	{
		float RadiusToScreen;        // radius
		float R2;     // 1/radius
		float NegInvR2;     // radius * radius
		float NDotVBias;

		glm::vec2 InvFullResolution;
		glm::vec2 InvQuarterResolution;

		float AOMultiplier;
		float PowExponent;
		glm::vec2 _pad0;

		glm::vec4 projInfo;
		glm::vec2 projScale;
		int projOrtho;
		int _pad1;

		glm::vec4 float2Offsets[AO_RANDOMTEX_SIZE * AO_RANDOMTEX_SIZE];
		glm::vec4 jitters[AO_RANDOMTEX_SIZE * AO_RANDOMTEX_SIZE];
	};

	SSAO(sceneData* _scd, AlgorithmType _algorithm = ALGORITHM_HBAO_CLASSIC,
			bool _blur = true, float _intensity = 8.f, float _blurSharpness = 30.0f);
	~SSAO();

	void bind();
	void clear(glm::vec4 clearCol = glm::vec4(0.f));
	void unbind();
	void copyFbo(camPar* cp);
	void proc(camPar* cp);
	void drawBlit(camPar* cp, bool copyDepth = false);
	void drawAlpha(camPar* cp, float alpha);

	void initShaders();
	void initFullScrQuad();
	void initBilateralblur();
	std::string initDepthLinearize(int msaa);
	void initViewNormal();
	std::string initHbaoCalc(int _blur, int deinterl);
	std::string initHbaoBlur(int _blur);
	void initDeinterleave();
	Shaders* initDebugDepth();
	std::string initReinterleave(int _blur);
	std::string initComputeUpdtShdr();
	GLuint createShaderPipelineProgram(GLuint target, const char* src);

	void prepareHbaoData(camPar* cp, int width, int height);
	void drawLinearDepth(camPar* cp, int width, int height, int sampleIdx);
	void drawHbaoBlur(int width, int height, int sampleIdx);
	void drawHbaoClassic(camPar* cp, int width, int height, int sampleIdx);
	void drawHbaoCacheAware(camPar* cp, int width, int height, int sampleIdx);

	bool initMisc();
	bool initFramebuffers(int width, int height, int samples);

	inline void newBuffer(GLuint &glid)
	{
		if (glid)
			glDeleteBuffers(1, &glid);
		glGenBuffers(1, &glid);
	}

	inline void newTexture(GLuint &glid)
	{
		if (glid)
			glDeleteTextures(1, &glid);
		glGenTextures(1, &glid);
	}

	inline void newFramebuffer(GLuint &glid)
	{
		if (glid)
			glDeleteFramebuffers(1, &glid);
		glGenFramebuffers(1, &glid);
	}

	void checkFboStatus();

private:
	AlgorithmType algorithm;
	bool blur;
	float blurSharpness;
	float intensity;
	int samples;

	sceneData* scd;

	ShaderCollector* shCol;

	SceneData sceneUbo;
	HBAOData hbaoUbo;
	glm::vec4 hbaoRandom[HBAO_RANDOM_ELEMENTS * MAX_SAMPLES];
	SsaoPar tweak;

	Quad* quad;

	ShaderBuffer<glm::vec4>* modu_pos;
	ShaderBuffer<glm::vec4>* m_vel;
	ShaderBuffer<glm::vec4>* ref_pos;
	CameraControl m_control;

	Shaders* depth_linearize;
	Shaders* depth_linearize_msaa;
	Shaders* viewnormal;
	Shaders* bilateralblur;
	Shaders* texShdr;

	Shaders* hbao_calc;
	Shaders* hbao_calc_blur;
	Shaders* hbao_blur;
	Shaders* hbao_blur2;

	Shaders* hbao2_deinterleave;
	Shaders* hbao2_calc;
	Shaders* hbao2_calc_blur;
	Shaders* hbao2_reinterleave;
	Shaders* hbao2_reinterleave_blur;

	Shaders* debugDepth;

	bool inited = false;

	std::string vertShdr;
	std::string basicVert;
	std::string com;
	std::string fullScrQuad;
	std::string fullScrQuadGeo;
	std::string shdr_Header;

	GLint lastBoundFbo;
	GLuint defaultVAO;
	GLuint hbao_ubo;

	float propo;
	double lastUpdt;
};
}
