//
// TessTerrain.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <glm/gtc/matrix_access.hpp>

#include "GeoPrimitives/Quad.h"
#include "math_utils.h"

#ifndef SHDR_MAPPING
#define SHDR_MAPPING 1

#define ALIGN(a)__attribute__ ((aligned (a)))

// Matrices, must align to 4 vector (16 bytes)
#define SDK_MAT4 ALIGN(16) glm::mat4

///@{
/// vectors.
/// vectors, 4-tuples and 3-tuples must align to 16 bytes
///  2-vectors must align to 8 bytes
#define SDK_VEC4 ALIGN(16) glm::vec4
#define SDK_VEC3 ALIGN(16) glm::vec3
#define SDK_VEC2 ALIGN(8) glm::vec2
#define SDK_BOOL ALIGN(4) bool

#endif

namespace tav
{
class TessTerrain
{
public:
	struct TessellationParams
	{
		float innerTessFactor;
		float outerTessFactor;

		float noiseFreq;
		int noiseOctaves;
		float invNoiseSize;
		float invNoise3DSize;
		float heightScale;

		float triSize;
		SDK_VEC4 viewport;

		SDK_MAT4 ModelView;
		SDK_MAT4 ModelViewProjection;
		SDK_MAT4 Projection;
		SDK_MAT4 InvProjection;
		SDK_MAT4 InvView;

		bool smoothNormals;
		bool cull;
		bool lod;
		SDK_VEC3 lightDir;
		SDK_VEC3 lightDirWorld;
		SDK_VEC4 eyePosWorld;

		SDK_VEC4 frustumPlanes[6];

		float time;
		SDK_VEC2 translate;

		int gridW;
		int gridH;
		SDK_VEC3 tileSize;
		SDK_VEC3 gridOrigin;
		float tileBoundingSphereR;

		float invFocalLen;

		TessellationParams() :
		innerTessFactor(32.0f),
		outerTessFactor(32.0f),
		noiseFreq(0.25f),
		heightScale(3.7f),
		noiseOctaves(8),
		triSize(5.0f),
		cull(true),
		lod(true),
		smoothNormals(true),
		time(0.0f),
		gridW(16),
		gridH(16),
		invNoiseSize(0.1f),
		invNoise3DSize(0.1f),
		invFocalLen(0.1f)
		{
			gridOrigin = glm::vec3(-8.0f, 0.0f, -8.0f);
			tileSize = glm::vec3(1.0f, 0.0f, 1.0f);
			lightDir = glm::vec3(0.0f, 0.0f, -1.0f);
			lightDirWorld = glm::vec3(0.0f, 0.0f, -1.0f);
			glm::vec3 halfTileSize = glm::vec3(tileSize.x, heightScale, tileSize.z)*0.5f;
			tileBoundingSphereR = glm::length(halfTileSize);
		}
	};

	TessTerrain(ShaderCollector* _shCol, unsigned int _quality=2);
	~TessTerrain();

	void draw(double time, double dt, glm::mat4& projMat,
			glm::mat4& viewMat, glm::mat4& modelMat, glm::vec2 actFboSize, TFO* _tfo=0);

	void loadShaders();
	void initUniformHeader();
	void initNoiseFunc();
	void initNoise3DFunc();
	void initTerrainFunc();
	void initGenTerrainVs();
	void initGenTerrainFs();
	void initTerrainVertex();
	void initTerrainControl();
	void initTerrainTess();
	void initTerrainFrag();
	void initWireFrameGeo();
	void initWireFrameFrag();
	void initSkyVs();
	void initSkyFs();

	GLuint createShaderPipelineProgram(GLuint target, const char* src);
	const char* GetShaderStageName(GLenum target);
	void checkProgPipeline();

	// void updateTerrainTex();
	void computeFrustumPlanes(glm::mat4 &viewMatrix, glm::mat4 &projMatrix, glm::vec4 *plane);
	bool sphereInFrustum(glm::vec3 pos, float r, glm::vec4 *plane);
	void drawTerrain(TFO* _tfo);
	void drawSky(TFO* _tfo);
	void updateQuality();

	GLuint createNoiseTexture2D(int w, int h, GLint internalFormat, bool mipmap=false);
	GLuint createNoiseTexture2DNeighbours(int w, int h, GLuint internalFormat);
	GLuint createNoiseTexture3D(int w, int h, int d, GLint internalFormat, bool mipmap=false);
	GLuint createNoiseTexture4f3D(int w, int h, int d, GLint internalFormat, bool mipmap=false);

	void setHeight( float _val );
	void setAnimSpeed( float _val );
	void setSkyTop( float _val );
	void setSkyHeight( float _val );
private:
	ShaderCollector* shCol;
	Quad* quad;
	Shaders* shdr;
	Shaders* mGenerateTerrainProg;
	Shaders* mSkyProg;
	//FBO*			mTerrainFbo;
	VAO* testVAO;

	glm::vec3 mLightDir;

	std::string shdr_Header;
	std::string noise_func;
	std::string noise3D_func;
	std::string terrain_func;
	std::string uniforms_header;
	std::string generate_terrain_vs;
	std::string generate_terrain_fs;
	std::string terrain_vertex;
	std::string terrain_control;
	std::string terrain_tess;
	std::string terrain_frag;
	std::string wireframe_geometry;
	std::string fragment_wireframe;
	std::string sky_vs;
	std::string sky_fs;

	GLuint mTerrainPipeline = 0;

	GLuint mTerrainVertexProg = 0;
	GLuint mTerrainTessControlProg = 0;
	GLuint mTerrainTessEvalProg = 0;
	GLuint mWireframeGeometryProg = 0;
	GLuint mTerrainFragmentProg = 0;
	GLuint mWireframeFragmen = 0;
	GLuint mWireframeFragmentProg = 0;

	TessellationParams mParams;

	unsigned int mQuality;

	//uniform buffer object
	GLuint mUBO;
	//vertex buffer object
	GLuint mVBO;

	GLuint mGPUQuery;
	GLuint mRandTex;
	GLuint mRandTex3D;
	GLuint mNumPrimitives;

	bool mCull;
	bool mLod;
	bool mAnimate;
	bool mWireframe;

	float skyTop = 11.f;
	float skyHeight = 10.f;
	float heightScale = 3.f;
	float animSpeed = 0.f;
};
}
