//
//  GLSLParticleSystem2.h
//  tav_gl4
//
//  Created by Sven Hahne on 24.09.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#ifndef __tav_gl4__GLSLParticleSystem2__
#define __tav_gl4__GLSLParticleSystem2__

#include <stdio.h>

#include "headers/global_vars.h"
#include "math_utils.h"
#include "GLUtils/GLMCamera.h"
//#include "SceneNode.h"
#include "Shaders/ShaderCollector.h"
#include "GLUtils/TextureBuffer.h"
#include "GLUtils/TFO.h"
#include "GLUtils/UniformBlock.h"
#include "GLUtils/VAO.h"

namespace tav
{
class GLSLParticleSystem2
{
public:
	enum drawType
	{
		POINTS = 0, QUADS = 1, DT_COUNT = 2
	};
	enum emitType
	{
		PET_DATA = 0, PET_TEXTURE = 1, PET_COUNT = 2
	};

	typedef struct
	{
		glm::vec3 emitOrg;
		glm::vec3 emitVel;
		float speed = 1.f;
		float size = 0.1f;
		float angle = 0.f;
		float life = 2.f;
		int texUnit = 0;
		int texNr = 0;
		glm::vec3 posRand;
		float dirRand = 0.f;
		float speedRand = 0.f;
		float sizeRand = 0.f;
		float angleRand = 0.f;
		float texRand = 0.f;
		float lifeRand = 0.f;
		float colInd = 0.f;
		float alpha = 1.f;
	} EmitData;

	typedef struct
	{
		GLint texNr;
		int width;
		int height;
		int emitLimit;
		float size;
		float speed = 1.f;
		float angle = 0.f;
		float colInd = 0.f;
		float alpha = 1.f;
		glm::vec3* massCenter;
	} EmitTexData;

	GLSLParticleSystem2(ShaderCollector* _shCol, int _maxNrParticles,
			int _screenWidth, int _screenHeight);
	~GLSLParticleSystem2();
	void init(TFO* _tfo = nullptr);
	void emit(double time, unsigned int nrPart, EmitData& _data, bool remember =
			false, GLint posTex = 0, int texWidth = 0, int texHeight = 0,
			glm::vec3* massCenter = nullptr);
	void emitFromData(double time, double _medDt);
	void emitFromTex(double time, double _medDt);
	//void clearEmitBuf();
	void update(double time, bool draw = false, GLint velTex = 0,
			drawType drT = POINTS);
	void draw(GLenum _mode = GL_POINTS, TFO* _tfo = nullptr);
	void drawToBlend(drawType drT = POINTS, TFO* _tfo = nullptr);

	void bindStdShader(drawType drT = POINTS);
	Shaders* getStdShader(drawType drT = POINTS);

	Shaders* initUpdateShdr(unsigned int ind = 0);
	Shaders* initEmitShdr(unsigned int ind = 0);
	Shaders* initEmitTexShdr(unsigned int ind = 0);
	Shaders* initQuadShdr();
	Shaders* initVelTexShdr();
	Shaders* initOrgTexShdr();
	Shaders* initReturnToOrgShdr();
	Shaders* initQuadSceneNodeBlendShdr();

	void setCheckBounds(float _check);
	void setLifeTime(float _lifeTime);
	void setGravity(glm::vec3 _gravity);
	void setAging(bool _age);
	void setFriction(float _friction);
	void setAgeFading(bool _set);
	void setAgeSizing(bool _set);
	void setColorPal(glm::vec4* chanCols);
	void setVelTexAngleStepSize(float _size);
	void setReturnToOrg(bool _set, float _reposFact = 0.f);
	void copyEmitData(EmitData& fromData, EmitData& toData);

	VAO* emitVao;

private:
	void updateVelTex(GLint velTex);
	void updateVelTexOrg(GLint velTex, double time);
	void updateReturnToOrg(double time);

	GLMCamera* cam;
	TFO** tfos;
	TFO* velUptTfo;
	TFO* velOrgUptTfo;
	TFO* posUptTfo;
	VAO* part;
	VAO* emitTrigVao;
	Shaders** updateShaders;
	Shaders** emitShaders;
	Shaders** emitTexShaders;
	Shaders* quadShader;
	Shaders* quadSceneNodeBlendShdr;
	Shaders* velTexShader;
	Shaders* velOrgTexShader;
	Shaders* returnToOrgShader;
	ShaderCollector* shCol;

	coordType* tfoBufAttachTypes;

	TextureBuffer* velUpdtTb;
	TextureBuffer** initStateTb;

	//UniformBlock**  ub;

	unsigned int nrEmitTbos;
	unsigned int nrEmitTbosCoords;
	unsigned int maxNrParticles;
	unsigned int nrTfos;
	unsigned int tfoPtr;
	unsigned int nrNewEmit = 0;
	GLuint nrEmittedParticles = 0;

	float velTexAngleStepSize;
	float reposFact;
	float lifeTime;
	float doAging;
	float friction;
	float ageFading;
	float ageSizing;
	// float           orgRand = 0.003f;
	float colorInd;
	glm::vec4* colorPal;
	glm::vec3 gravity;

	unsigned int* emitPtr;
	unsigned int* emitCntr;
	std::vector<std::pair<unsigned int, unsigned int> > emittedNrPart;
	bool resetEmits = false;

	int screenWidth;
	int screenHeight;
	int maxNrEmitTrig;

	bool isInited = false;
	bool returnToOrg = false;
	bool checkBounds = false;

	const char* vShader;
	const char* fShader;

	const char* qvShader;
	const char* qgShader;
	const char* qfShader;

	std::vector<std::string> recAttribNames;

	double lastTime = 0.0;
	double medDt = 0.0;

	int* texNrs;

	std::map<double, int> partDieEvents;
	emitType actEmitType;
	EmitTexData actEmitTexData;

	GLuint g_transformFeedbackQuery = 0;
};
}

#endif /* defined(__tav_gl4__GLSLParticleSystem2__) */
