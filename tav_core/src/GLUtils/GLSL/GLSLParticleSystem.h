//
//  GLSLParticleSystem.h
//  tav_gl4
//
//  Created by Sven Hahne on 24.09.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#ifndef __tav_gl4__GLSLParticleSystem__
#define __tav_gl4__GLSLParticleSystem__

#include <stdio.h>
#include <string>

#include "headers/gl_header.h"
#include "headers/global_vars.h"
#include "math_utils.h"
#include "GLUtils/GLMCamera.h"
//#include "SceneNode.h"
#include "Shaders/ShaderCollector.h"
#include "GLUtils/TextureBuffer.h"
#include "GLUtils/TFO.h"
#include "GLUtils/VAO.h"

namespace tav
{
class GLSLParticleSystem
{
public:
	enum drawType
	{
		POINTS = 0, QUADS = 1, DT_COUNT = 2
	};

	typedef struct
	{
		glm::vec3 emitOrg;
		glm::vec3 emitVel;
		glm::vec4 emitCol;
		float speed = 1.f;
		float size = 0.1f;
		float angle = 0.f;
		float life = 2.f;
		int texUnit = 0;
		int texNr = 0;
		int maxNrTex;
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
	} EmitData;

	GLSLParticleSystem(ShaderCollector* _shCol, int _maxNrParticles,
			int _screenWidth, int _screenHeight);
	~GLSLParticleSystem();
	void init(TFO* _tfo = nullptr);
	void emit(unsigned int nrPart, EmitData& _data, bool remember = false,
			GLint posTex = 0, int texWidth = 0, int texHeight = 0);
	void clearEmitBuf();
	void update(double time, bool draw = false, GLint velTex = 0, drawType drT =
			POINTS);
	void draw(GLenum _mode = GL_POINTS, TFO* _tfo = nullptr);
	void drawToBlend(drawType drT = POINTS, TFO* _tfo = nullptr);

	void bindStdShader(drawType drT = POINTS);

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

private:
	void updateVelTex(GLint velTex);
	void updateEmitPosTex(GLint emitTex, int texWidth, int texHeight);
	void updateReturnToOrg(double time);

	GLMCamera* cam;
	TFO** tfos;
	TFO* velUptTfo;
	TFO* posUptTfo;
	TFO* emitPosUptTfo;
	VAO* part;
	VAO* emitTrigVao;
	Shaders** partRecShaders;
	Shaders* quadShader;
	Shaders* quadSceneNodeBlendShdr;
	Shaders* velTexShader;
	Shaders* returnToOrgShader;
	Shaders* emitTexShader;
	ShaderCollector* shCol;

	coordType* tfoBufAttachTypes;

	TextureBuffer** emitTbs;
	TextureBuffer* velUpdtTb;
	TextureBuffer** initStateTb;

	unsigned int nrEmitTbos;
	unsigned int nrEmitTbosCoords;
	unsigned int maxNrParticles;
	unsigned int nrTfos;
	unsigned int tfoPtr;

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
	int nrEmitTrig;

	bool isInited = false;
	bool returnToOrg = false;

	const char* vShader;
	const char* fShader;

	const char* qvShader;
	const char* qgShader;
	const char* qfShader;

	std::vector<std::string> recAttribNames;

	double lastTime = 0.0;
	double medDt = 0.0;

	int* texNrs;
};
}

#endif /* defined(__tav_gl4__GLSLParticleSystem__) */
