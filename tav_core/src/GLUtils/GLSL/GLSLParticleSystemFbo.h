//
//  GLSLParticleSystemFbo.h
//  tav_core
//
//  Created by Sven Hahne on 15/7/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <stdio.h>
#include <cmath>
#include <map>

#include "headers/global_vars.h"
#include "headers/tav_types.h"
#include "math_utils.h"
#include "GLUtils/GLMCamera.h"
#include "GLUtils/PingPongFbo.h"
#include "Shaders/ShaderCollector.h"
#include "GLSLParticleSystem.h"
#include "GLUtils/TextureBuffer.h"
#include "GLUtils/TFO.h"
#include "GLUtils/UniformBlock.h"
#include "GLUtils/VAO.h"

namespace tav
{
class GLSLParticleSystemFbo
{
public:
	enum drawType
	{
		POINTS = 0, QUADS = 1, DT_COUNT = 2
	};
	enum texBufType
	{
		TBT_POSITION = 0,
		TBT_VELOCITY = 1,
		TBT_COLOR = 2,
		TBT_AUX0 = 3,
		TBT_COUNT = 4
	};
	std::string samplerNames[4] =
	{ "pos_tex", "vel_tex", "col_tex", "aux0_tex" };
	std::string emitNames[4] =
	{ "emitTboPos", "emitVel", "emitCol", "emitAux0" };

	typedef struct
	{
		glm::vec3 emitOrg = glm::vec3(0.f);
		glm::vec3 emitVel = glm::vec3(0.f, 1.f, 0.f);
		glm::vec4 emitCol = glm::vec4(1.f);
		float speed = 1.f;
		float size = 0.1f;
		float angle = 0.f;
		float life = 2.f;
		float posRand = 0.f;
		float dirRand = 0.f;
		float speedRand = 0.f;
		float sizeRand = 0.f;
		float angleRand = 0.f;
		float texRand = 0.f;
		float lifeRand = 0.f;
		float colRand = 0.f;
		float colInd = 0.f;
		float alpha = 1.f;
		float extVelAmt = 0.5f;
		float ageFadeCurv = 0.5f;
		int texUnit = 0;
		int maxNrTex = 4;
		GLint texNr = 0;
		GLint velTexNr = 0;
		GLint depthTexNr = 0;
		int width = 0;
		int height = 0;
		int dWidth = 0;
		int dHeight = 0;
		glm::vec3* massCenter;
		glm::ivec2 nrCells = glm::ivec2(0);
		glm::vec2 texIndOffs = glm::vec2(0.f);
		int totNrCells;
		unsigned int emitNrPart = 0;
	} EmitData;

	typedef struct
	{
		glm::ivec2 cellSize;
		glm::vec2 cellStep;
		glm::ivec2 nrCells;
		glm::vec2 fNrCells;
		glm::ivec2 tqCellSize;
		glm::ivec2 nrTqCells;
		glm::ivec2 texSize;
		glm::vec2 fTexSizeInv;
		glm::vec2 fTexSize;
		glm::vec2 texIndScaleF;
		glm::vec2 texIndOffs;
		unsigned int maxNrPart;
		unsigned int* cellWriteOffs;
		float* fCellWriteOffs;
		glm::ivec2 cellPtr;
		unsigned int nrPartPerCell;
		float fNrPartPerCell;
		unsigned int nrPartPerTqCell;
		float fNrPartPerTqCell;
		unsigned int totNrCells;
		unsigned int totNrTqCells;
		//TextureManager* cellOffsTex;
		TextureBuffer* cellOffsTexBuf;
		PingPongFbo* ppBuf;
		VAO* emitDataVao = 0;
		VAO* emitTexVao = 0;
		TFO* emitTexTfo = 0;
		VAO* trigVaoPoints = 0;
		VAO* trigVaoQuads = 0;
	} partTex;

	GLSLParticleSystemFbo(ShaderCollector* _shCol, int _nrParticles,
			float _scrAspect);
	~GLSLParticleSystemFbo();

	void initPartTex(partTex* ptex, unsigned int _nrParticles,
			unsigned int _maxGeoAmpPoints);
	void bindTexBufs(Shaders* _shader);
	void draw();
	void draw(GLfloat* matPtr);
	void drawQuad(GLfloat* matPtr, TFO* _tfo, Shaders* _shader,
			GLuint normTexNr, GLuint litsphereTexture);
	void drawQuadDirect(GLfloat* matPtr, GLuint texNr, GLuint normTexNr,
			GLuint litsphereTexture);
	void drawMult(int nrCam, glm::mat4* _modMatr, glm::vec4* _viewPorts);
	void update(double time);
	void update(double time, GLint velTex, GLint extForceTex = -1);
	void emit(unsigned int nrPart);
	void emit(unsigned int nrPart, GLint emitTex, int width, int height,
			GLint velTex = 0);
	void procEmission();
	void procEmissionTex();
	void procEmissionDepthTex(unsigned int _nrEmitPart);
	void updateWriteOffsets();

	void setEmitData(GLSLParticleSystemFbo::EmitData* _data);
	void setEmitTexThres(float _emitTexThres);
	void setLifeTime(float _lifeTime);
	void setGravity(glm::vec3 _gravity);
	void setAging(bool _age);
	void setFriction(float _friction);
	void setAgeFading(bool _set);
	void setAgeSizing(bool _set);
	void setColorPal(glm::vec4* chanCols);
	void setVeloBright(float _val);
	glm::vec3 RGBtoHSV(float r, float g, float b);
	glm::vec3 HSVtoRGB(float h, float s, float v);

	GLint* getCellSizePtr();
	GLint getMaxGeoAmpPoints();
	GLuint getActPosTex();
	GLuint getActVelTex();
	VAO* getDrawTrig();
	GLuint getPTexSrcId(unsigned int _attNr);
	GLuint getEmitTex();
	unsigned int getNrAttachments();

	void initUpdateShdr();
	void initVelTexShdr();
	void initEmitShdr();
	void initEmitTexRecShdr();
	void initEmitTexShdr();
	void initEmitDepthTexShdr();
	void initDrawPointShdr();
	void initDrawQuadShdr();
	void initDrawQuadShdrDirect();
	void initDrawMultShdr();

	int maxNrCellSqrtQuadAmp;
	int nrPartSqrt;
	VAO* quadDrawVao;

	partTex basePTex;
	GLint maxGeoAmpTriStrip;
	GLint maxGeoAmpPoints;
	GLint glMaxGeoAmpPoints;

private:
	ShaderCollector* shCol;
	Quad* quad;
	Quad* instQuads;

	Shaders* updateShader;
	Shaders* updateVelTexShader = 0;
	Shaders* emitShader = 0;
	Shaders* emitShaderTex = 0;
	Shaders* emitShaderTexRec = 0;
	Shaders* emitShaderDepthTex = 0;
	Shaders* drawShader = 0;
	Shaders* drawShaderQuad = 0;
	Shaders* drawShaderQuadDirect = 0;
	Shaders* drawMultShader = 0;
	Shaders* stdColShdr;
	Shaders* stdTexShdr;

	UniformBlock* emitShaderTexRecUBlock = 0;
	UniformBlock* emitShaderTexUBlock = 0;

	TextureBuffer** emitBufs;
	FBO* scaledEmitTex = 0;
	EmitData* actEmitData = 0;

	std::vector<GLuint> emitTexIndices;
	unsigned int emitTexNrIndices = 0;

	bool drawQuadShaderInited = false;
	bool useAtomicCounters = false;

	GLuint query = 0;
	GLuint primitives;
	GLuint atomicBuffer;

	int nrAttachments;
	int nrTexBufCoord;
	int lastNrEmitPart;
	int emitIndStep = 1;
	unsigned int emitLimOffs = 0;
	unsigned int lastNrPrimitives = 0;

	float lifeTime = 1.f;
	float doAging;
	float friction = 0.f;
	float ageFading = 0.f;
	float ageSizing = 0.f;
	float colorInd = 0.f;
	float scrAspect;
	float emitTexThres = 0.001f;
	float veloBright = 1.f;

	double lastTime = 0.0;
	double medDt = 0.0;

	glm::vec4* colorPal;
	glm::vec3 gravity;

	std::string stdVert;
	std::string shdr_Header;

	std::vector<coordType> tfoBufAttachTypes;
	std::vector<std::string> recAttribNames;
};
}
