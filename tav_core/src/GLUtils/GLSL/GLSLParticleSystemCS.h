//
//  GLSLGLSLParticleSystemCSCS.h
//  tav_gl4
//
//  Created by Sven Hahne on 24.09.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#ifndef __tav_gl4__GLSLParticleSystemCS__
#define __tav_gl4__GLSLParticleSystemCS__

#pragma once

#include <stdlib.h>
#include <iostream>
#include <glm/gtx/string_cast.hpp>

#include "headers/gl_header.h"
#include "math_utils.h"

#include "Shaders/ShaderCollector.h"
#include "Shaders/ShaderBuffer.h"
#include "GLUtils/NoiseTexNV.h"
#include "GLUtils/GLSL/NTreeSortCS.h"

namespace tav
{

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
	ShaderParams() :
			spriteSize(0.015f), attractor(0.0f, 0.0f, 0.0f, 0.0f), damping(
					0.95f), noiseFreq(10.0f), initAmt(1.f), noiseStrength(
					0.001f)
	{
	}
};

class GLSLParticleSystemCS
{
public:
	enum emitMode { EMIT_NONE=0, EMIT_INIT_PARS=1, EMIT_TEX_INIT_PARS=2 };
	enum drawMode { DRAW_POINT=0, DRAW_GEN_QUADS=1, DRAW_TEX_QUADS=2 };

	GLSLParticleSystemCS(size_t size, ShaderCollector* _shCol, drawMode _dMode,
			bool createNoiseTex=true, unsigned int _work_group_size=128);
	~GLSLParticleSystemCS();

	// par_bufs have to be set before calling these methods!!!
	void setUpdateShader();
	void setUpdateShader(const char* src);
	void setEmitShader(bool _randEmit=false);
	void setEmitTexShader();
	void setAging(bool _val) { aging = _val; }

	void randomInitBuf(std::string buf_name, glm::vec4 randScale, glm::vec4 randOffs);
	void zero();

	void update(double time, double dt);

	void emit(unsigned int nrPart, glm::vec4* _initPars);
	void emit(unsigned int nrPart, glm::vec4* _initPars, glm::vec4* _randAmt);
	void emit(unsigned int nrPart, glm::vec4* _initPars, GLint _emitTex,
			unsigned int width, unsigned int height);

	void procEmit(float time);
	void procEmitInitPars(float time);
	void procEmitTexInitPars(float time);

	void addBuffer(std::string name) {
		buf_index_cntr++;
		par_bufs[name] = std::make_pair(buf_index_cntr, new ShaderBuffer<glm::vec4>(m_size));
		if (initPars) delete initPars;
		if (randAmt) delete randAmt;
		initPars = new glm::vec4[par_bufs.size()];
		randAmt = new glm::vec4[par_bufs.size()];
	}

	bool getAging() { return aging; }
	size_t getSize() { return m_size; }
	GLuint getNoiseTex() { return m_noiseTex; }
	int getNoiseTexSize() { return m_noiseSize; }
	unsigned int getWorkGroupSize(){ return work_group_size; }
	Shaders* getUpdtShdr(){ return updtShader; }
	float getEmitTexActivity(){
		if(quadSorter) return quadSorter->getTreeAt(0); else return 0.f;
	}

	ShaderBuffer<glm::vec4>* getBuffer(std::string buf_name)
	{
		if ( par_bufs.find(buf_name) == par_bufs.end() ) {
			return 0;
		} else {
			return par_bufs[buf_name].second;
		}
	}
	ShaderBuffer<uint32_t>* getIndexBuffer() { return m_indices; }

	void bindBuffers(){
		for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin(); it != par_bufs.end(); ++it)
		{
			//std::cout << "update bind " << (*it).first << " to " << (*it).second.first << std::endl;
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, (*it).second.first, (*it).second.second->getBuffer());
		}
	}
	void unbindBuffers(){
		for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin(); it != par_bufs.end(); ++it)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, (*it).second.first, 0);
	}
	void bindNoiseTexture(GLuint unit=0){
		glActiveTexture(GL_TEXTURE0+unit);
		glBindTexture(GL_TEXTURE_3D, m_noiseTex);
	}

private:
	ShaderParams 										mShaderParams;
	ShaderCollector*									shCol;
	Shaders*											updtShader=0;
	Shaders*											emitShader=0;
	Shaders*											emitRandShader=0;
	Shaders*											emitTexShader=0;

	NTreeSortCS*										lineSorter=0;
	NTreeSortCS*										quadSorter=0;

	size_t 												m_size;

	std::map< std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>* > >	par_bufs;
	ShaderBuffer<uint32_t>*								m_indices;

	GLuint 												m_programPipeline;
	GLuint 												m_programEmit;
	GLuint 												m_updateProg;
	GLuint 												m_texEmitProg;

	GLuint 												emitTex;
	GLuint 												m_noiseTex;
	GLuint												noiseTexIndOffs=0;
	int 												m_noiseSize;
	NoiseTexNV* 										nt;

	bool												aging;
	bool												randEmit;

	unsigned int 										m_size_sqrt;
	unsigned int										nrPartToEmit;
	unsigned int 										work_group_size;
	unsigned int										buf_index_cntr;

	// int work_group_size = 128;
	drawMode											drMode;

	emitMode											requestEmit=EMIT_NONE;
	glm::vec4* 											initPars;
	glm::vec4* 											randAmt;
	glm::ivec3											noiseTexSize;
	glm::ivec2											emitTexSize;
};

}

#endif /* defined(__tav_gl4__GLSLParticleSystemCS__) */
