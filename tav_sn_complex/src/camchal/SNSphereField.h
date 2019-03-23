//
// SNSphereField.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GLUtils/TextureManager.h>
#include "GLUtils/Vertex.h"
#include "GLUtils/Geometry.h"
#include "GLUtils/CameraControl.h"
#include "GLUtils/MersenneTwister.h"
#include <math_utils.h>
#include "Shaders/ShaderBuffer.h"
#include "Shaders/ShaderCollector.h"
#include "GLUtils/Noise3DTexGen.h"
#include <GeoPrimitives/Quad.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <glm/ext.hpp>
#include <glm/gtx/color_space.hpp>


#define DEBUG_FILTER     1

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

class ResourceGLuint {
public:
  GLuint  m_value;

  ResourceGLuint() : m_value(0) {}

  ResourceGLuint( GLuint b) : m_value(b) {}
  operator GLuint() const { return m_value; }
  operator GLuint&() { return m_value; }
  ResourceGLuint& operator=( GLuint b) { m_value = b; return *this; }
};

#endif

class SNSphereField : public SceneNode
{
public:
	static const int SAMPLE_SIZE_WIDTH = 1280;
	static const int SAMPLE_SIZE_HEIGHT = 720;

	static const int SAMPLE_MAJOR_VERSION = 4;
	static const int SAMPLE_MINOR_VERSION = 3;

	static const int NUM_MRT = 8;
	static const int HBAO_RANDOM_SIZE = AO_RANDOMTEX_SIZE;
	static const int HBAO_RANDOM_ELEMENTS = HBAO_RANDOM_SIZE*HBAO_RANDOM_SIZE;
	static const int MAX_SAMPLES = 8;

	static const int gridX = 64;
	static const int gridY = 10;
	static const int gridZ = 4;
	const float      globalscale = 16.0f;

	enum AlgorithmType {
		ALGORITHM_NONE,
		ALGORITHM_HBAO_CACHEAWARE,
		ALGORITHM_HBAO_CLASSIC,
		NUM_ALGORITHMS,
	};

    struct {
      ResourceGLuint
        scene,
        depthlinear,
        viewnormal,
        hbao_calc,
        hbao2_deinterleave,
        hbao2_calc;
    } fbos;

    struct {
      ResourceGLuint
        scene_vbo,
        scene_ibo,
        scene_ubo,
        hbao_ubo;
    } buffers;

    struct {
      ResourceGLuint
        scene_color,
        scene_depthstencil,
        scene_depthlinear,
        scene_viewnormal,
        hbao_result,
        hbao_blur,
        hbao_random,
        hbao_randomview[MAX_SAMPLES],
        hbao2_deptharray,
        hbao2_depthview[HBAO_RANDOM_ELEMENTS],
        hbao2_resultarray;
    } textures;

	struct SsaoPar {
		SsaoPar()
		: algorithm(ALGORITHM_HBAO_CLASSIC)
		, samples(1)
		, intensity(2.f)
		, radius(2.f)
		, bias(0.1f)
		, blur(1)
		, blurSharpness(40.0f)
		, ortho(false)
		{}

		int             samples;
		AlgorithmType   algorithm;
		float           intensity;
		float           bias;
		float           radius;
		int             blur;
		float           blurSharpness;
		bool            ortho;
	};

	struct SceneData {
		glm::mat4  viewProjMatrix;
		glm::mat4  viewMatrix;
		glm::mat4  viewMatrixIT;

		glm::uvec2 viewport;
		glm::uvec2 _pad;
	};

	// corresponds to the uniform block -> later more elegantglm::
	struct HBAOData {
		float   		RadiusToScreen;        // radius
		float   		R2;     // 1/radius
		float   		NegInvR2;     // radius * radius
		float   		NDotVBias;

		glm::vec2    	InvFullResolution;
		glm::vec2    	InvQuarterResolution;

		float   		AOMultiplier;
		float   		PowExponent;
		glm::vec2    	_pad0;

		glm::vec4    	projInfo;
		glm::vec2    	projScale;
		int     		projOrtho;
		int     		_pad1;

		glm::vec4    	float2Offsets[AO_RANDOMTEX_SIZE*AO_RANDOMTEX_SIZE];
		glm::vec4    	jitters[AO_RANDOMTEX_SIZE*AO_RANDOMTEX_SIZE];
	};

	struct Projection {
		float nearplane;
		float farplane;
		float fov;
		float orthoheight;
		bool  ortho;
		glm::mat4  matrix;

		Projection()
		: nearplane(0.1f)
		, farplane(100.0f)
		, fov((40.f))
		, orthoheight(1.0f)
		, ortho(false)
		{ }

		void update(int width, int height){
			float aspect = float(width) / float(height);
			if (ortho){
				matrix = glm::ortho(-orthoheight*0.5f*aspect, orthoheight*0.5f*aspect, -orthoheight*0.5f, orthoheight*0.5f, nearplane, farplane);
			}
			else{
				matrix = glm::perspective(fov, aspect, nearplane, farplane);
			}
		}
	};


	SNSphereField(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNSphereField();

	void init(TFO* _tfo = nullptr);
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);

	void initShaders();
	void initSceneShdr();
	void initFullScrQuad();
	void initBilateralblur();
	std::string initDepthLinearize(int msaa);
	void initViewNormal();
	void initDisplayTex();
	std::string initHbaoCalc(int blur, int deinterl);
	std::string initHbaoBlur(int blur);
	void initDeinterleave();
	std::string initReinterleave(int blur);
	std::string initComputeUpdtShdr();
	GLuint createShaderPipelineProgram(GLuint target, const char* src);

	void think(double time);
	//void resize(int width, int height);

	void prepareHbaoData(const Projection& projection, int width, int height);

	void drawLinearDepth(const Projection& projection, int width, int height, int sampleIdx);
	void drawHbaoBlur(const Projection& projection, int width, int height, int sampleIdx);
	void drawHbaoClassic(const Projection& projection, int width, int height, int sampleIdx);
	void drawHbaoCacheAware(const Projection& projection, int width, int height, int sampleIdx);

	bool initScene();
	bool initMisc();
	bool initFramebuffers(int width, int height, int samples);

	void updateOffsPos(double time);
	void udpateLiquid(double time);
	glm::vec2 toFluidCoord(glm::vec2 normPos);

	inline void newBuffer(GLuint &glid)
	{
		if (glid) glDeleteBuffers(1,&glid);
		glGenBuffers(1,&glid);
	}

	inline void newTexture(GLuint &glid)
	{
		if (glid) glDeleteTextures(1,&glid);
		glGenTextures(1,&glid);
	}

	inline void newFramebuffer(GLuint &glid)
	{
		if (glid) glDeleteFramebuffers(1,&glid);
		glGenFramebuffers(1,&glid);
	}

	void checkFboStatus();
	void startThreads(double time, double dt);
	void stopThreads(double time, double dt);
	void cleanUp();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};

private:
	TextureManager* 	litsphereTex;
	Projection 			projection;

	SceneData  			sceneUbo;
	HBAOData   			hbaoUbo;
	glm::vec4      		hbaoRandom[HBAO_RANDOM_ELEMENTS * MAX_SAMPLES];
	SsaoPar      		tweak;
	glm::uint  			sceneTriangleIndices;
	glm::uint  			sceneObjects;

    Noise3DTexGen*		noiseTex;
    Quad*				quad;

	ShaderBuffer<glm::vec4>* modu_pos;
	ShaderBuffer<glm::vec4>* m_vel;
	ShaderBuffer<glm::vec4>* ref_pos;
	CameraControl 		m_control;

    ShaderCollector*	shCol;

	Shaders*			draw_scene;
	Shaders*			depth_linearize;
	Shaders*			depth_linearize_msaa;
	Shaders*			viewnormal;
	Shaders*			bilateralblur;
	Shaders*			displaytex;
	Shaders*			texShdr;

	Shaders*			hbao_calc;
	Shaders*			hbao_calc_blur;
	Shaders*			hbao_blur;
	Shaders*			hbao_blur2;

	Shaders*			hbao2_deinterleave;
	Shaders*			hbao2_calc;
	Shaders*			hbao2_calc_blur;
	Shaders*			hbao2_reinterleave;
	Shaders*			hbao2_reinterleave_blur;

	bool                inited = false;

	std::string			vertShdr;
	std::string 		basicVert;
	std::string 		com;
	std::string 		fullScrQuad;
	std::string 		fullScrQuadGeo;
	std::string 		shdr_Header;

	GLuint				defaultVAO;
    GLuint 				m_programPipeline;
    GLuint 				m_updateProg;
    GLint 				lastBoundFbo = 0;

	unsigned int		sphereIndOffs;
    unsigned int    	flWidth;
    unsigned int    	flHeight;
    glm::vec2    		fluidSize;
    GLSLFluid*      	fluidSim;
    glm::vec2       	oldPos;
    glm::vec2      		forcePos;
    glm::vec4*			chanCols;
    int 				m_noiseSize;

	float				propo;

	float				spSize= 1.f;
	float				fluidForce= 20.f;
	float				perlForce= 1.f;
	float				randAmt= 0.f;
	float				timeScale= 0.1f;
	float				oscSpeed= 1.f;
	float				oscAmt= 1.f;
	float				xPos= 0.f;
	float				posRandSpd= 1.f;
	float				posRandAmtX= 1.f;
	float				posRandAmtY= 1.f;
	float				velScale= 0.3f;
	float				rad = 0.2f;
	float				timeStep = 0.025f;

	double				lastUpdt;
};
}
