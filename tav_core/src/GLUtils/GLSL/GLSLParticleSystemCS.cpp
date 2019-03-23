/**
*
*  GLSLParticleSystemCS.cpp
*  tav_gl4
*
*	Particlesystem with ComputeShaders
*
*	Class to manage an array of buffers with parameters
*	Contains a line-tree sorting mechanism to find free indices to emit new particles
*
*	Standard configuration is a single position buffer in the format (x,y,z,lifetime) (lifetime =0 -> particle dead)
*	A line tree will be generated holding the number of free indexes for each cell and hierarchy, when a emit is requested
*
*	Since the number of buffers and their meaning is free, it's no possible to have a general update-shader,
*	although the process of binding the buffer and activating the shader is always the same (done by the "update" method)
*	THE UPDATE SHADER HAS TO BE SET FROM OUTSIDE
*
*	The process of emitting particles can be generalized and is provided by the "emit"-methods
*	Emitting can be done by:
*		- "manually" writing of values into the buffers (setting lifetime > 0) and calling the update method (thus possibly
*		  overwriting active particles)
*		- requesting an amount of particles to emit, letting the classes quad-tree sorting mechanism search for free indexes
*		  ("lifetime" == 0), providing an external Buffer with initial values.
*		- requesting an amount of particles to emit, estimating free indexes, just providing vec4s with fixed inital values
*		  or scaling values for random values read from a noise texture (options for different noise algorithms)
*		- requesting an amount of particles to emit, use a b/w texture to define the initial positions of the particles.
*		  Free Indexes are estimated. Vec4s with fixed initial values or scaling values for random values read from a noise
*		  texture (options for different noise algorithms) have to be provided
*
**/

#include "pch.h"
#include "GLSLParticleSystemCS.h"

#define STRINGIFY(A) #A

namespace tav
{

GLSLParticleSystemCS::GLSLParticleSystemCS(size_t size, ShaderCollector* _shCol,
		drawMode _drMode, bool createNoiseTex, unsigned int _work_group_size) :
	m_noiseTex(0), m_noiseSize(48), m_updateProg(0), shCol(_shCol), drMode(_drMode),
	work_group_size(_work_group_size), buf_index_cntr(0), aging(true)
{
	// show limits
	/*
	int cx, cy, cz;
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &cx );
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &cy );
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &cz );
	printf("Max compute work group count = %d, %d, %d\n", cx, cy, cz );

	int sx, sy, sz;
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &sx );
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &sy );
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &sz );
	printf("Max compute work group size  = %d, %d, %d\n", sx, sy, sz );
	*/

	//-----------------------------------------------------------------------

	m_size = (size_t)(std::log(size) / std::log(2));
	m_size = (size_t)std::pow(2.0, (double)m_size);
	//std::cout << "GLSLParticleSystemCS:: m_size: " << m_size << std::endl;

	//-----------------------------------------------------------------------

	// init the par_bufs with a position buffer
	par_bufs["pos"] = std::make_pair(0, new ShaderBuffer<glm::vec4>(m_size));
	initPars = new glm::vec4[par_bufs.size()];
	randAmt = new glm::vec4[par_bufs.size()];

	// if the desired draw mode is QUAD, generate a Buffer with indices to
	// draw the points as quads
	if (drMode == DRAW_GEN_QUADS || drMode == DRAW_TEX_QUADS)
	{
		// generate an index buffer for the quad rendering option
		m_indices = new ShaderBuffer<uint32_t>(m_size * 6);

		// the index buffer is a classic "two-tri quad" array.
		// This may seem odd, given that the compute buffer contains a single
		// vector for each particle.  However, the shader takes care of this
		// by indexing the compute shader buffer with a /4.  The value mod 4
		// is used to compute the offset from the vertex site, and each of the
		// four indices in a given quad references the same center point
		uint32_t *indices = m_indices->map();
		for (size_t i=0; i<m_size; i++)
		{
			size_t index = i << 2;
			*(indices++) = index;
			*(indices++) = index + 1;
			*(indices++) = index + 2;
			*(indices++) = index;
			*(indices++) = index + 2;
			*(indices++) = index + 3;
		}
		m_indices->unmap();
	}

	//-----------------------------------------------------------------------

	// raw noise
	if (createNoiseTex)
	{
		nt = new tav::NoiseTexNV(m_noiseSize, m_noiseSize, m_noiseSize, GL_RGBA8_SNORM);
		m_noiseTex = nt->getTex();
		noiseTexSize = glm::ivec3(m_noiseSize, m_noiseSize, m_noiseSize);
	}

	lineSorter = new NTreeSortCS(2, m_size, ".a < 0.001", shCol);
}

//-----------------------------------------------------------------------

void GLSLParticleSystemCS::setEmitShader(bool _randEmit)
{
	// load emit shader
	std::string src = shCol->getShaderHeader();

	int ind = 0;
	for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin(); it != par_bufs.end(); ++it)
	{
		std::string bufName = (*it).first;
		bufName[0] = std::toupper(bufName[0]);
		src += "layout( std140, binding="+std::to_string((*it).second.first +1)+" ) buffer "+ bufName + "{ vec4 " + (*it).first + "[]; };\n";
		src += "uniform vec4 init_" + (*it).first + ";\n";
		src += "uniform vec4 randInit_" + (*it).first + ";\n";
		ind++;
	}

	src += "layout(local_size_x="+std::to_string(work_group_size)+", local_size_y=1, local_size_z=1) in;\n";
	src += lineSorter->getReadParameterStr(0, "part");	// add the layouts and uniform needed by the tree sorter

	if (_randEmit)
		src +=  "uniform float time;\n"
				"uniform sampler3D noiseTex3D;\n"
				"uniform float numParticles;\n";

	// add the layouts and uniform needed by the tree sorter
	src +=  "void main(){\n"
			"\t uint i = gl_GlobalInvocationID.x;\n";

	src += lineSorter->getReadBodyStr("i", "writeInd");

	if (_randEmit)
		src +=  "float fInd = float(writeInd) / numParticles;\n"
				"vec4 randVal = texture(noiseTex3D, vec3(fInd * time, time * (fInd + 0.15), time * (fInd + 0.052)));\n";

	for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin(); it != par_bufs.end(); ++it)
	{
		src += (*it).first + "[writeInd] = init_" + (*it).first;
		if (!_randEmit)
			src += ";\n";
		else
			src += " + vec4(randVal.xyz, (randVal.x + randVal.y)) * randInit_" + (*it).first + ";\n";
	}

	src += "}";

	emitShader = shCol->addCheckShaderText("GLSLParticleSystemCS_emit_rand_"+std::to_string(_randEmit), src.c_str());
}

//-----------------------------------------------------------------------

void GLSLParticleSystemCS::setEmitTexShader()
{
	// load emit shader
	std::string src = "#version 430\n";

	for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin(); it != par_bufs.end(); ++it)
	{
		std::string bufName = (*it).first;
		bufName[0] = std::toupper(bufName[0]);
		src += "layout( std140, binding="+std::to_string((*it).second.first +2)+" ) buffer "+ bufName + "{ vec4 " + (*it).first + "[]; };\n";
		src += "uniform vec4 init_" + (*it).first + ";\n";
	}

	src += "layout(local_size_x="+std::to_string(work_group_size)+", local_size_y=1, local_size_z=1) in;\n";

	src += lineSorter->getReadParameterStr(0, "part");	// add the layouts and uniform needed by the tree sorter
	src += quadSorter->getReadParameterStr(1, "etex");	// add the layouts and uniform needed by the tree sorter

	//----------------------------------------------------------------------------------

	// add the layouts and uniform needed by the tree sorter
	src += STRINGIFY(
	uniform sampler2D velTex;\n
	uniform sampler3D noiseTex3D;\n
	uniform ivec3 noiseTex3DSize;\n
	uniform ivec2 srcTexSize;\n
	uniform vec2 emitTexStepSize;\n
	uniform uint noiseTexIndOffs;\n
	\n
	void main(){\n
		uint i = gl_GlobalInvocationID.x;\n
	\n);

	//----------------------------------------------------------------------------------

	// add the sorters function to get indices from the sorted tree with dead particles
	src += lineSorter->getReadBodyStr("i", "writeInd");

	//----------------------------------------------------------------------------------

	// main part, generate random offsets to the found positions
	src += STRINGIFY(
	// generate an index into the noise texture that is different each iteration
	uint indOffs = i + noiseTexIndOffs;\n
	// randomValue -1 to 1
	vec4 randVal = texelFetch(noiseTex3D, ivec3(indOffs % noiseTex3DSize.x, \n
										  \t\t (indOffs / noiseTex3DSize.x) % noiseTex3DSize.y, \n
										  \t\t indOffs / (noiseTex3DSize.x * noiseTex3DSize.y)), \n
						\t 0);\n
	// move randVal into 0 -1
	randVal = randVal * vec4(0.5) + vec4(0.5);\n
	indOffs = i;\n
	if (etex_nrPartToEmit < etex_sortBuffer[0]) {\n
		// generate an offset to the input index to have the emit part always covered
		\t uint emitOffsStep = uint(float(etex_sortBuffer[0]) / float(etex_nrPartToEmit));\n
		\t indOffs = i * emitOffsStep + (uint(float(noiseTexIndOffs) * randVal.r) % emitOffsStep);\n
	}\n);

	//----------------------------------------------------------------------------------

	// add the sorters function to get indices from the sorted tree with red spots in the emit Texture
	src += quadSorter->getReadBodyStr("indOffs", "tex_readInd");

	//----------------------------------------------------------------------------------

	// we got sorted indices. now set the values - for the emitTexture convert the indices back to positions
	for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin(); it != par_bufs.end(); ++it)
	{
		if ((*it).second.first == 0){
			// convert the tex_readInd to the position it corresponds to in the emitTex
			// if we are emitting less particles than we have available add a random offset
			src += (*it).first + "[writeInd] = vec4( float(tex_readInd % srcTexSize.x) / float(srcTexSize.x) * 2.0 -1.0,\n";
			src += "\t float(tex_readInd / srcTexSize.x) / float(srcTexSize.y) * 2.0 -1.0,\n";
			src += "\t 0.0, init_" + (*it).first + ".a);\n";
			src += (*it).first + "[writeInd].xy += randVal.xy * emitTexStepSize;\n";
			src += (*it).first + "[writeInd].xyz += init_" + (*it).first + ".xyz;\n";

		} else {
			src += (*it).first + "[writeInd] = init_" + (*it).first + ";\n";
		}
	}

	src += "}";

	emitTexShader = shCol->addCheckShaderText("GLSLParticleSystemCS_emit_tex", src.c_str());
}

//----------------------------------------------------

void GLSLParticleSystemCS::setUpdateShader()
{
	if (updtShader) delete updtShader;

	// version statement kommt von aussen
	std::string src = shCol->getShaderHeader();

	for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin(); it != par_bufs.end(); ++it)
	{
		std::string bufName = (*it).first;
		bufName[0] = std::toupper(bufName[0]);
		src += "layout( std140, binding="+std::to_string((*it).second.first)+" ) buffer "+ bufName + "{ vec4 "+(*it).first+"[]; };\n";
	}

	src += "layout(local_size_x="+std::to_string(work_group_size)+", local_size_y=1, local_size_z=1) in;\n";

	src += STRINGIFY(
	uniform float dt;\n
	uniform float time;\n
	uniform uint numParticles;\n
	uniform uint aging;\n

	// compute shader to update particles
	void main() {\n

		uint i = gl_GlobalInvocationID.x;\n

		// thread block size may not be exact multiple of number of particles
		if (i >= numParticles) return;\n

		// read particle position and velocity from buffers
		vec3 p = pos[i].xyz;\n
		vec3 v = vel[i].xyz;\n

		// integrate
		p += v;\n
		//v *= damping;\n

		// write new values
		pos[i] = vec4(p, max(pos[i].a - dt, 0.0));\n // lower lifetime
//		pos[i] = vec4(p, aging == 0 ? pos[i].w : max(pos[i].a - dt, 0.0));\n // lower lifetime
		vel[i] = vec4(v, 0.0);\n
	});

	updtShader = shCol->addCheckShaderText("GLSLParticleSystemCS_updt_shdr_std", src.c_str());
}

//------------------------------------------------------------------------------------
// set custom update shader from outside the class

void GLSLParticleSystemCS::setUpdateShader(const char* src)
{
	// generate updateProg
	if (updtShader) delete updtShader;

	// generate unique name
	char* shdrName = new char[100];
	sprintf(shdrName, "GLSLParticleSystemCS_updt_shdr%p", this);

	updtShader = shCol->addCheckShaderText(shdrName, src);

	updtShader->begin();
	updtShader->setUniform1f("invNoiseSize", 1.0f / m_noiseSize);
	updtShader->setUniform1i("noiseTex3D", 0);
	updtShader->end();
}

//------------------------------------------------------------------------------------

GLSLParticleSystemCS::~GLSLParticleSystemCS()
{
	glDeleteProgram(m_updateProg);
	glDeleteProgram(m_texEmitProg);
}

//------------------------------------------------------------------------------------

void GLSLParticleSystemCS::randomInitBuf(std::string buf_name, glm::vec4 randScale,
		glm::vec4 randOffs)
{
	GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

	if ( par_bufs.find(buf_name) != par_bufs.end() )
	{
		glm::vec4* buf = par_bufs[buf_name].second->map(access);
		for (size_t i=0; i<m_size; i++)
			buf[i] = glm::vec4(	tav::sfrand() * randScale.x + randOffs.x,
								tav::sfrand() * randScale.y + randOffs.y,
								tav::sfrand() * randScale.z + randOffs.z,
								tav::sfrand() * randScale.w + randOffs.w);
		par_bufs[buf_name].second->unmap();
	}
}

//------------------------------------------------------------------------------------

void GLSLParticleSystemCS::zero()
{
	GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

	for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin(); it != par_bufs.end(); ++it)
	{
		glm::vec4* buf = (*it).second.second->map(access);
		for (size_t i=0; i<m_size; i++)
			buf[i] = glm::vec4(0.f, 0.f, 0.f, 0.f);
		(*it).second.second->unmap();
	}
}

//------------------------------------------------------------------------------------

// emit with init uniforms, must be a vec4 for each buf
void GLSLParticleSystemCS::emit(unsigned int nrPart, glm::vec4* _initPars)
{
	requestEmit = EMIT_INIT_PARS;
	randEmit = false;

	// make local copy
	memcpy(&initPars[0][0], &_initPars[0][0], par_bufs.size() * sizeof(glm::vec4));
	nrPartToEmit = nrPart;
}

//------------------------------------------------------------------------------------

void GLSLParticleSystemCS::emit(unsigned int nrPart, glm::vec4* _initPars, glm::vec4* _randAmt)
{
	requestEmit = EMIT_INIT_PARS;
	randEmit = true;

	// make local copy
	memcpy(&initPars[0][0], &_initPars[0][0], par_bufs.size() * sizeof(glm::vec4));
	memcpy(&randAmt[0][0], &_randAmt[0][0], par_bufs.size() * sizeof(glm::vec4));
	nrPartToEmit = nrPart;
}

//------------------------------------------------------------------------------------

// emit with an emit texture, init uniforms, must be a vec4 for each buf
void GLSLParticleSystemCS::emit(unsigned int nrPart, glm::vec4* _initPars, GLint _emitTex,
		unsigned int _width, unsigned int _height)
{
	requestEmit = EMIT_TEX_INIT_PARS;

	if (!quadSorter){
		quadSorter = new NTreeSortCS(4, _emitTex, _width, _height, ".r > 0.1", shCol);
		emitTexSize = glm::ivec2(_width, _height);
	}

	if (!emitTexShader) setEmitTexShader();

	memcpy(&initPars[0][0], &_initPars[0][0], par_bufs.size() * sizeof(glm::vec4));
	nrPartToEmit = nrPart;
	emitTex = _emitTex;
}

//------------------------------------------------------------------------------------

void GLSLParticleSystemCS::procEmit(float time)
{

	switch (requestEmit){
	case EMIT_INIT_PARS :
		procEmitInitPars(time);
		break;
	case EMIT_TEX_INIT_PARS:
		procEmitTexInitPars(time);
		break;
	default:
		break;
	}

	requestEmit = EMIT_NONE;
}

//------------------------------------------------------------------------------------

void GLSLParticleSystemCS::procEmitInitPars(float time)
{
	unsigned int nrWorkgroups = (unsigned int)std::max((double)nrPartToEmit / (double) work_group_size, 1.0);

	lineSorter->sort(par_bufs["pos"].second->getBuffer());

	emitShader->begin();

	if (randEmit)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, m_noiseTex);

		emitShader->setUniform1i("noiseTex3D", 0);
		emitShader->setUniform1f("numParticles", (float)m_size);
		emitShader->setUniform1f("time", time);
	}

	lineSorter->setReadUniforms(emitShader, nrPartToEmit);
	lineSorter->bindSortBufferBase(0);

	for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin();
			it != par_bufs.end(); ++it)
	{
		emitShader->setUniform4fv("init_"+(*it).first, &initPars[(*it).second.first][0] );
		if (randEmit) emitShader->setUniform4fv("randInit_"+(*it).first, &randAmt[(*it).second.first][0] );
	}

	for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin();
			it != par_bufs.end(); ++it)
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, (*it).second.first +1, (*it).second.second->getBuffer());
	}

	glDispatchCompute(nrWorkgroups, 1, 1);

	// We need to block here on compute completion to ensure that the
	// computation is done before we render
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin();
			it != par_bufs.end(); ++it)
	{
		glBindBufferBase( GL_SHADER_STORAGE_BUFFER, (*it).second.first +1, 0);
	}

	emitShader->end();

/*
	std::cout << "----------------------------- emit debug ---------------" << std::endl;

	// debug emit:
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, par_bufs["aux0"].second->getBuffer());
	glm::vec4* ptr = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * m_size, GL_MAP_READ_BIT);

	for (int i=0; i<m_size; i++)
		std::cout << "aux0[" << i <<  "]" << glm::to_string(ptr[i]) << std::endl;

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	std::cout << "----------------------------- emit done ---------------" << std::endl;
	*/
}

//------------------------------------------------------------------------------------

void GLSLParticleSystemCS::procEmitTexInitPars(float time)
{
	unsigned int nrWorkgroups = (unsigned int)std::max((double)nrPartToEmit / (double) work_group_size, 1.0);

	// process the treesorters for getting empty particle indices and white spots on the emit texture
	lineSorter->sort(par_bufs["pos"].second->getBuffer());
	quadSorter->sort();

	emitTexShader->begin();
	emitTexShader->setUniform1i("velTex", 0);
	emitTexShader->setUniform1i("noiseTex3D", 1);
	emitTexShader->setUniform3iv("noiseTex3DSize", &noiseTexSize[0]);
	emitTexShader->setUniform2iv("srcTexSize", &emitTexSize[0]);
	emitTexShader->setUniform1ui("noiseTexIndOffs", (unsigned int)getRandF(0, m_size));
	emitTexShader->setUniform2f("emitTexStepSize", 2.f / (float)emitTexSize.x, 2.f / (float)emitTexSize.y);

	lineSorter->setReadUniforms(emitTexShader, nrPartToEmit);
	lineSorter->bindSortBufferBase(0);
	quadSorter->setReadUniforms(emitTexShader, nrPartToEmit);
	quadSorter->bindSortBufferBase(1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, nt->getTex());


	for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin();
			it != par_bufs.end(); ++it)
	{
		//std::cout << "emit bind " << "init_"+(*it).first<< " to initPars[" << (*it).second.first << "]" << glm::to_string(initPars[(*it).second.first]) << std::endl;
		emitTexShader->setUniform4fv("init_"+(*it).first, &initPars[(*it).second.first][0] );
	}

	for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin();
			it != par_bufs.end(); ++it)
	{
	//	std::cout << "emit bind " << (*it).first << " to " << (*it).second.first +2 << std::endl;
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, (*it).second.first +2, (*it).second.second->getBuffer());
	}

	glDispatchCompute(nrWorkgroups, 1, 1);

	// We need to block here on compute completion to ensure that the
	// computation is done before we render
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin();
			it != par_bufs.end(); ++it)
	{
		glBindBufferBase( GL_SHADER_STORAGE_BUFFER, (*it).second.first +2, 0);
	}

	emitTexShader->end();

	noiseTexIndOffs += nrPartToEmit;	// an offset to the read position in the noise tex -> must be different each iteration to get "real" noise

	//std::cout << "noiseTexIndOffs: " << noiseTexIndOffs << std::endl;

/*
	std::cout << "----------------------------- emit debug ---------------" << std::endl;

	// debug emit:
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, (*par_bufs.begin()).second.second->getBuffer());
	glm::vec4* ptr = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * m_size, GL_MAP_READ_BIT);

	for (int i=0; i<m_size; i++)
		std::cout << "pos[" << i <<  "]" << glm::to_string(ptr[i]) << std::endl;

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	std::cout << "----------------------------- emit done ---------------" << std::endl;
	*/
}

//------------------------------------------------------------------------------------

void GLSLParticleSystemCS::update(double time, double dt)
{
	procEmit((float)time);

	// Invoke the compute shader to integrate the particles
	updtShader->begin();
	updtShader->setUniform1f("dt", (float)dt);
	updtShader->setUniform1f("time", (float)time);
	updtShader->setUniform1ui("numParticles", (unsigned int)m_size);
	updtShader->setUniform1ui("aging", (unsigned int)aging);

	//std::cout << "numParticles " << m_size << std::endl;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, m_noiseTex);

	for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin(); it != par_bufs.end(); ++it)
	{
	//	std::cout << "update bind " << (*it).first << " to " << (*it).second.first << std::endl;
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, (*it).second.first, (*it).second.second->getBuffer());
	}

	glDispatchCompute((unsigned int)std::max((double)m_size / (double)work_group_size, 1.0), 1, 1);

	// We need to block here on compute completion to ensure that the
	// computation is done before we render
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	for(std::map<std::string, std::pair<unsigned short, ShaderBuffer<glm::vec4>*> >::iterator it = par_bufs.begin(); it != par_bufs.end(); ++it)
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, (*it).second.first, 0);

	updtShader->end();


	/*

	// debug update
	std::cout << "----------------------------- update ---------------" << std::endl;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, (*par_bufs.begin()).second.second->getBuffer());
	glm::vec4* ptr = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * m_size, GL_MAP_READ_BIT);

	for (int i=0; i<m_size; i++)
		std::cout << "pos[" << i <<  "]" << glm::to_string(ptr[i]) << std::endl;

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	*/
}

}
