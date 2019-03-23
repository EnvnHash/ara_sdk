//
// SNAudioCompPart.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  die "update" methode muss aufgerufen werden!!!

#include "SNAudioCompPart.h"

#define STRINGIFY(A) #A

namespace tav
{

SNAudioCompPart::SNAudioCompPart(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs), mReset(false), m_noiseSize(16), m_curlNoiseProg(
				0), spriteSize(0.01f), initAmt(0.f)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	pa = (PAudio*) scd->pa;
	osc = (OSCData*) scd->osc;

	// setup chanCols
	getChanCols(&chanCols);

	// get tex0
	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];

	// get screen Proportions from roomDim
	propo = _scd->roomDim->x / _scd->roomDim->y;

	addPar("alpha", &alpha);
	addPar("spriteSize", &spriteSize);
	addPar("initAmt", &initAmt);

	// setup custom matrix
	globalScale = 16.f;
	view = glm::lookAt(glm::vec3(0.f, 0.f, globalScale),	// camPos
	glm::vec3(0.f), 			// lookAt
	glm::vec3(0.f, 1.f, 0.f));	// upVec

	//float aspect = float(scd->screenWidth) / float(scd->screenHeight);
	float near = globalScale * 0.1f;
	float far = globalScale * 10.f;
	proj = glm::perspective(0.2f, propo, near, far);

	// vao mit irgendwas drin, damit die sache mit dem shader storage buffer richtig funktioniert
	testVAO = new VAO("position:3f", GL_STATIC_DRAW);
	GLfloat pos[] =
			{ -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f,
					0.5f, 0.0f };
	testVAO->upload(POSITION, pos, 4);
	GLuint ind[] =
	{ 0, 1, 3, 1, 3, 2 };
	testVAO->setElemIndices(6, ind);

	//create ubo and initialize it with the structure data
	glGenBuffers(1, &mUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderParams), &mShaderParams,
			GL_STREAM_DRAW);

	glGenBuffers(1, &eUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, eUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(EmitParams), &mEmitParams,
			GL_STREAM_DRAW);

	//create simple single-vertex VBO
	float vtx_data[] =
	{ 0.0f, 0.0f, 0.0f, 1.0f };
	glGenBuffers(1, &mVBO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_data), vtx_data, GL_STATIC_DRAW);

	// Split the velocities and positions, as for now the
	// rendering only cares about positions, so we just index
	// that buffer for rendering
	m_pos = new ShaderBuffer<glm::vec4>*[scd->nrChannels];
	m_init_pos = new ShaderBuffer<glm::vec4>*[scd->nrChannels];
	m_vel = new ShaderBuffer<glm::vec4>*[scd->nrChannels];
	m_acc = new ShaderBuffer<glm::vec4>*[scd->nrChannels];
	m_lifespans = new ShaderBuffer<float>*[scd->nrChannels];
	m_emitterIds = new ShaderBuffer<int>*[scd->nrChannels];
	pitchMed = new Median<float>*[scd->nrChannels];

	prevPos = new glm::vec3[scd->nrChannels];
	prevVel = new glm::vec3[scd->nrChannels];
	prevTime = new double[scd->nrChannels];

	for (int i = 0; i < scd->nrChannels; i++)
	{
		m_pos[i] = new ShaderBuffer<glm::vec4>(mNumParticles);
		m_init_pos[i] = new ShaderBuffer<glm::vec4>(mNumParticles);
		m_vel[i] = new ShaderBuffer<glm::vec4>(mNumParticles);
		m_acc[i] = new ShaderBuffer<glm::vec4>(mNumParticles);
		m_lifespans[i] = new ShaderBuffer<float>(mNumParticles);
		m_emitterIds[i] = new ShaderBuffer<int>(mNumParticles);
		pitchMed[i] = new Median<float>(30.f);
		prevPos[i] = glm::vec3(0.f);
		prevVel[i] = glm::vec3(0.f);
		prevTime[i] = 0.0;
	}

	// the index buffer is a classic "two-tri quad" array.
	// This may seem odd, given that the compute buffer contains a single
	// vector for each particle.  However, the shader takes care of this
	// by indexing the compute shader buffer with a /4.  The value mod 4
	// is used to compute the offset from the vertex site, and each of the
	// four indices in a given quad references the same center point
	m_indices = new ShaderBuffer<uint32_t>(mNumParticles * 6);

	uint32_t *indices = m_indices->map();
	for (size_t i = 0; i < mNumParticles; i++)
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

	//  load particle curlNoise shader 

	glGenProgramPipelines(1, &m_programCurlNoise);
	std::string src = initCurlShdr();
	m_curlNoiseProg = createShaderPipelineProgram(GL_COMPUTE_SHADER,
			m_programCurlNoise, src.c_str());
	glBindProgramPipeline(m_programCurlNoise);
	glBindProgramPipeline(0);

	//  load particle emitt shader 

	glGenProgramPipelines(1, &m_programEmit);
	src = initEmitShdr();
	m_emitProg = createShaderPipelineProgram(GL_COMPUTE_SHADER, m_programEmit,
			src.c_str());
	glBindProgramPipeline(m_programEmit);

//	GLint loc = glGetUniformLocation(m_curlNoiseProg, "invNoiseSize");
//	glProgramUniform1f(m_curlNoiseProg, loc, 1.0f / m_noiseSize);
//
//	loc = glGetUniformLocation(m_curlNoiseProg, "noiseTex3D");
//	glProgramUniform1i(m_curlNoiseProg, loc, 0);

	glBindProgramPipeline(0);

	// 

	// init particle positions
	reset(glm::vec3(propo, 1.f, 1.f));

	//  ShaderParams 

	mShaderParams.numParticles = mNumParticles;
}

//----------------------------------------------------

void SNAudioCompPart::initDrawShdr()
{
	std::string shdr_Header = "#version 430\n#define WORK_GROUP_SIZE "
			+ std::to_string(work_group_size) + "\n";

	std::string vert =
			STRINGIFY(
					layout(std140, binding=1) uniform ShaderParams { mat4 ModelView; mat4 ModelViewProjection; mat4 ProjectionMatrix; vec4 attractor; uint numParticles; float spriteSize; float damping; float noiseFreq; vec3 noiseStrength;

					float time; float persistence; float NOISE_POSITION_SCALE; float NOISE_SCALE; float NOISE_TIME_SCALE; };

					layout( std140, binding=1 ) buffer Pos { vec4 pos[]; };

					uniform vec4[6] colors; uniform sampler3D noiseCube; uniform int chanNr; uniform float alpha;

					out gl_PerVertex { vec4 gl_Position; };

					out block { vec4 color; vec2 texCoord; } Out;

					void main() {
					// expand points to quads without using GS
					int particleID = gl_VertexID >> 2;// 4 vertices per particle
					vec4 particlePos = pos[particleID];

					//Out.color = mix(colors[1], colors[2], 1.0 );
					Out.color = colors[chanNr];
//		Out.color = mix(colors[1], colors[2], min ( max( texture(noiseCube, particlePos.xyz * 0.1) + 0.3, 0.0), 1.0));

					//map vertex ID to quad vertex
					vec2 quadPos = vec2( ((gl_VertexID - 1) & 2) >> 1, (gl_VertexID & 2) >> 1);

					vec4 particlePosEye = ModelView * particlePos; vec4 vertexPosEye = particlePosEye + vec4((quadPos*2.0-1.0)*spriteSize, 0, 0);

					Out.texCoord = quadPos;

					gl_Position = ProjectionMatrix * vertexPosEye; });

	vert = shdr_Header + vert;

	//

	shdr_Header = "#version 430\n";
//	shdr_Header = "#version 430\n#extension GL_EXT_shader_io_blocks : enable\n";

	std::string frag =
			STRINGIFY(in block { vec4 color; vec2 texCoord; } In;

			layout(location=0) out vec4 fragColor; uniform float alpha;

			void main() {
			// Quick fall-off computation
					float r = length(In.texCoord*2.0-1.0)*3.0; float i = exp(-r*r); if (i < 0.01) discard;

					fragColor = vec4(In.color.rgb, i * alpha); });

	frag = "// SNAudioCompPart frag\n" + shdr_Header + frag;

	printf("init draw shader \n");

	mRenderProg = shCol->addCheckShaderText("ParticleCSShader3234",
			vert.c_str(), frag.c_str());
}

//----------------------------------------------------

GLuint SNAudioCompPart::createShaderPipelineProgram(GLuint target, GLuint prog,
		const char* src)
{
	GLuint object;
	GLint status;

	const GLchar* fullSrc[2] =
	{ "#version 430\n", src };
	object = glCreateShaderProgramv(target, 2, fullSrc); // with this command GL_PROGRAM_SEPARABLE is set to true

	{
		GLint logLength;
		glGetProgramiv(object, GL_INFO_LOG_LENGTH, &logLength);
		char *log = new char[logLength];
		glGetProgramInfoLog(object, logLength, 0, log);
		//  printf("Shader pipeline program not valid:\n%s\n", log);
		delete[] log;
	}

	glBindProgramPipeline(prog);
	glUseProgramStages(prog, GL_COMPUTE_SHADER_BIT, object);
	glValidateProgramPipeline(prog);
	glGetProgramPipelineiv(prog, GL_VALIDATE_STATUS, &status);

	if (status != GL_TRUE)
	{
		GLint logLength;
		glGetProgramPipelineiv(prog, GL_INFO_LOG_LENGTH, &logLength);
		char *log = new char[logLength];
		glGetProgramPipelineInfoLog(prog, logLength, 0, log);
		printf("Shader pipeline not valid:\n%s\n", log);
		delete[] log;
	}

	glBindProgramPipeline(0);
	getGlError();

	return object;
}

//----------------------------------------------------

std::string SNAudioCompPart::getNoiseShaderFunctions()
{
	std::string out =
			STRINGIFY(
					vec4 mod289(vec4 x){ return x - floor(x * (1.0 / 289.0)) * 289.0; }\n float mod289(float x){ return x - floor(x * (1.0 / 289.0)) * 289.0; }\n vec4 permute(vec4 x){ return mod289(((x*34.0)+1.0)*x); }\n float permute(float x){ return mod289(((x*34.0)+1.0)*x); }\n vec4 taylorInvSqrt(vec4 r){ return 1.79284291400159 - 0.85373472095314 * r; }\n float taylorInvSqrt(float r){ return 1.79284291400159 - 0.85373472095314 * r; }\n

					vec4 grad4(float j, vec4 ip){\n const vec4 ones = vec4(1.0, 1.0, 1.0, -1.0); vec4 p; vec4 s; p.xyz = floor( fract (vec3(j) * ip.xyz) * 7.0) * ip.z - 1.0; p.w = 1.5 - dot(abs(p.xyz), ones.xyz); s = vec4(lessThan(p, vec4(0.0))); p.xyz = p.xyz + (s.xyz*2.0 - 1.0) * s.www; return p; }

					vec4 simplexNoiseDerivatives (vec4 v){ const vec4 C = vec4( 0.138196601125011,0.276393202250021,0.414589803375032,-0.447213595499958); vec4 i = floor(v + dot(v, vec4(F4)) ); vec4 x0 = v - i + dot(i, C.xxxx); vec4 i0; vec3 isX = step( x0.yzw, x0.xxx ); vec3 isYZ = step( x0.zww, x0.yyz ); i0.x = isX.x + isX.y + isX.z; i0.yzw = 1.0 - isX; i0.y += isYZ.x + isYZ.y; i0.zw += 1.0 - isYZ.xy; i0.z += isYZ.z; i0.w += 1.0 - isYZ.z; vec4 i3 = clamp( i0, 0.0, 1.0 ); vec4 i2 = clamp( i0-1.0, 0.0, 1.0 ); vec4 i1 = clamp( i0-2.0, 0.0, 1.0 ); vec4 x1 = x0 - i1 + C.xxxx; vec4 x2 = x0 - i2 + C.yyyy; vec4 x3 = x0 - i3 + C.zzzz; vec4 x4 = x0 + C.wwww; i = mod289(i); float j0 = permute( permute( permute( permute(i.w) + i.z) + i.y) + i.x); vec4 j1 = permute( permute( permute( permute ( i.w + vec4(i1.w, i2.w, i3.w, 1.0 )) + i.z + vec4(i1.z, i2.z, i3.z, 1.0 )) + i.y + vec4(i1.y, i2.y, i3.y, 1.0 )) + i.x + vec4(i1.x, i2.x, i3.x, 1.0 )); vec4 ip = vec4(1.0/294.0, 1.0/49.0, 1.0/7.0, 0.0) ; vec4 p0 = grad4(j0, ip); vec4 p1 = grad4(j1.x, ip); vec4 p2 = grad4(j1.y, ip); vec4 p3 = grad4(j1.z, ip); vec4 p4 = grad4(j1.w, ip); vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3))); p0 *= norm.x; p1 *= norm.y; p2 *= norm.z; p3 *= norm.w; p4 *= taylorInvSqrt(dot(p4,p4)); vec3 values0 = vec3(dot(p0, x0), dot(p1, x1), dot(p2, x2)); //value of contributions from each corner at point
					vec2 values1 = vec2(dot(p3, x3), dot(p4, x4)); vec3 m0 = max(0.5 - vec3(dot(x0,x0), dot(x1,x1), dot(x2,x2)), 0.0);//(0.5 - x^2) where x is the distance
					vec2 m1 = max(0.5 - vec2(dot(x3,x3), dot(x4,x4)), 0.0); vec3 temp0 = -6.0 * m0 * m0 * values0; vec2 temp1 = -6.0 * m1 * m1 * values1; vec3 mmm0 = m0 * m0 * m0; vec2 mmm1 = m1 * m1 * m1; float dx = temp0[0] * x0.x + temp0[1] * x1.x + temp0[2] * x2.x + temp1[0] * x3.x + temp1[1] * x4.x + mmm0[0] * p0.x + mmm0[1] * p1.x + mmm0[2] * p2.x + mmm1[0] * p3.x + mmm1[1] * p4.x; float dy = temp0[0] * x0.y + temp0[1] * x1.y + temp0[2] * x2.y + temp1[0] * x3.y + temp1[1] * x4.y + mmm0[0] * p0.y + mmm0[1] * p1.y + mmm0[2] * p2.y + mmm1[0] * p3.y + mmm1[1] * p4.y; float dz = temp0[0] * x0.z + temp0[1] * x1.z + temp0[2] * x2.z + temp1[0] * x3.z + temp1[1] * x4.z + mmm0[0] * p0.z + mmm0[1] * p1.z + mmm0[2] * p2.z + mmm1[0] * p3.z + mmm1[1] * p4.z; float dw = temp0[0] * x0.w + temp0[1] * x1.w + temp0[2] * x2.w + temp1[0] * x3.w + temp1[1] * x4.w + mmm0[0] * p0.w + mmm0[1] * p1.w + mmm0[2] * p2.w + mmm1[0] * p3.w + mmm1[1] * p4.w; return vec4(dx, dy, dz, dw) * 49.0; }

					vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }

					vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; } vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }

					float snoise(vec2 v){ const vec4 C = vec4(0.211324865405187, // (3.0-sqrt(3.0))/6.0
					0.366025403784439,// 0.5*(sqrt(3.0)-1.0)
					-0.577350269189626,// -1.0 + 2.0 * C.x
					0.024390243902439);// 1.0 / 41.0
					// First corner
					vec2 i = floor(v + dot(v, C.yy) ); vec2 x0 = v - i + dot(i, C.xx);
					// Other corners
					vec2 i1; i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0); vec4 x12 = x0.xyxy + C.xxzz; x12.xy -= i1;
					// Permutations
					i = mod289(i);// Avoid truncation effects in permutation
					vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 )) + i.x + vec3(0.0, i1.x, 1.0 )); vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0); m = m*m ; m = m*m ;
					// Gradients: 41 points uniformly over a line, mapped onto a diamond.
					// The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
					vec3 x = 2.0 * fract(p * C.www) - 1.0; vec3 h = abs(x) - 0.5; vec3 ox = floor(x + 0.5); vec3 a0 = x - ox;
					// Normalise gradients implicitly by scaling m
					// Approximation of: m *= inversesqrt( a0*a0 + h*h );
					m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
					// Compute final noise value at P
					vec3 g; g.x = a0.x * x0.x + h.x * x0.y; g.yz = a0.yz * x12.xz + h.yz * x12.yw; return 130.0 * dot(m, g); }\n);

	return out;
}

//----------------------------------------------------

std::string SNAudioCompPart::initCurlShdr()
{
	// version statement kommt von aussen
	std::string shdr_Header = "#define WORK_GROUP_SIZE "
			+ std::to_string(work_group_size) + "\n";
	shdr_Header += "#define F4 0.309016994374947451\n";

	std::string src =
			STRINGIFY(
					layout(std140, binding=1) uniform ShaderParams\n { mat4 ModelView;\n mat4 ModelViewProjection;\n mat4 ProjectionMatrix;\n vec4 attractor;\n uint numParticles;\n float spriteSize;\n float damping;\n float initAmt;\n float noiseFreq;\n float noiseStrength;\n

					float time; float persistence; float NOISE_POSITION_SCALE; float NOISE_SCALE; float NOISE_TIME_SCALE; };\n

					const int octaves = 3;

					layout( std140, binding=1 ) buffer Pos { vec4 pos[]; };\n layout( std140, binding=2 ) buffer Vel { vec4 vel[]; };\n layout( std140, binding=3 ) buffer Acc { vec4 acc[]; };\n layout( std140, binding=4 ) buffer Lifespans { float lifespan[]; };\n);

	src += "layout(local_size_x = " + std::to_string(work_group_size)
			+ ", local_size_y = 1, local_size_z = 1) in;\n";
	src += getNoiseShaderFunctions();

	// compute shader to update particles
	src +=
			STRINGIFY(
					void main() { uint gid = gl_GlobalInvocationID.x; vec3 oldPosition = pos[gid].xyz; vec3 noisePosition = oldPosition * NOISE_POSITION_SCALE; float noiseTime = time * NOISE_TIME_SCALE;

					vec4 xNoisePotentialDerivatives = vec4(0.0); vec4 yNoisePotentialDerivatives = vec4(0.0); vec4 zNoisePotentialDerivatives = vec4(0.0); float pers = persistence;

					for(int i = 0; i < octaves; ++i) { float scale = (1.0 / 2.0) * pow(2.0, float(i)); float noiseScale = pow(pers, float(i)); if(pers == 0.0 && i == 0){ //fix undefined behaviour
					noiseScale = 1.0; }

					xNoisePotentialDerivatives += simplexNoiseDerivatives(vec4(noisePosition * pow(2.0, float(i)), noiseTime)) * noiseScale * scale; yNoisePotentialDerivatives += simplexNoiseDerivatives(vec4((noisePosition + vec3(123.4, 129845.6, -1239.1)) * pow(2.0, float(i)), noiseTime)) * noiseScale * scale; zNoisePotentialDerivatives += simplexNoiseDerivatives(vec4((noisePosition + vec3(-9519.0, 9051.0, -123.0)) * pow(2.0, float(i)), noiseTime)) * noiseScale * scale; }

					//compute curl noise
					vec3 noiseVelocity = 50.0 * vec3(zNoisePotentialDerivatives[1] - yNoisePotentialDerivatives[2], xNoisePotentialDerivatives[2] - zNoisePotentialDerivatives[0], yNoisePotentialDerivatives[0] - xNoisePotentialDerivatives[1] ) * NOISE_SCALE; vec3 totalVelocity = vel[gid].xyz + noiseVelocity; vec3 newPosition = oldPosition + totalVelocity *0.1;

					pos[gid] = vec4(newPosition, 1.0);

					// regular update
					lifespan[gid] -= 1.0;\n

					vel[gid].xyz += acc[gid].xyz;\n pos[gid].xyz += vel[gid].xyz;\n acc[gid].xyz = vec3(0.0);\n

					});

	src = shdr_Header + src;

	return src;
}

//----------------------------------------------------

std::string SNAudioCompPart::getEmitterUtilityCode()
{
	std::string out =
			STRINGIFY(\n
			// Random float number between 0.0 and 1.0
					float rand_xorshift(){\n
					// Xorshift algorithm from George Marsaglia's paper
					rng_state ^= (rng_state << 13);\n rng_state ^= (rng_state >> 17);\n rng_state ^= (rng_state << 5);\n float r = float(rng_state) / 4294967296.0;\n return r;\n }\n \n
					// Random number between min(x, y) and max(x, y)
					float rand(float x, float y){\n float high = 0;\n float low = 0;\n float randNum = 0;\n if (x == y) return x;\n high = max(x,y);\n low = min(x,y);\n randNum = low + (high-low) * rand_xorshift();\n return randNum;\n }\n \n
					// Rotate a vec3 using a quaternion
					vec3 rotate(vec3 v, vec4 q){\n return v + 2.0 * cross( cross( v, q.xyz ) + q.w * v, q.xyz );\n }\n\n);
	return out;
}

//----------------------------------------------------

std::string SNAudioCompPart::getEmitterCode()
{
	std::string out =
			STRINGIFY(
			// Random lifespan based on emitter's parameters
					float newLifespan(){\n float var = rand(-lifespanVariation, lifespanVariation)/100.0;\n return averageLifespan*(1.0 + var);\n }\n \n

					// Random position based on emitter's parameters
					vec4 newPosition(){\n float theta = rand(0.0, 2.0*M_PI);\n
					// Uniform random point on sphere
					float u = rand(-1.0, 1.0);\n float x = radius *sqrt(1.0 -u *u) *cos(theta);\n float y = radius *sqrt(1.0 -u *u) *sin(theta);\n float z = radius *u;\n
					// float z = 0.0;\n
					vec3 intPos = mix(ipos.xyz, prevPos.xyz, rand(0.0, 1.0));\n vec3 newPos = intPos + vec3(x, y, z);\n return vec4(newPos + ipos.xyz, 1.0);\n }\n \n

					// Random velocity in a cone based on emitter's parameters
					vec4 newVelocity(){\n float phi = rand(0, 2.0 * M_PI);\n float nz = rand(cos(emissionAngle), 1.0);\n float nx = sqrt(1-nz*nz)*cos(phi);\n float ny = sqrt(1-nz*nz)*sin(phi);\n vec3 velDir = rotate(vec3(nx, ny, nz), orientation);\n float velNorm = averageVelocity*(1.0 + rand(-velocityVariation, velocityVariation)/100.0);\n vec3 newVel = velDir*velNorm;\n
					// newVel = velNorm*rotate(emissionDir, orientation);
					newVel = velNorm * rotate(vec3(nx, ny, nz), orientation);\n
					// newVel *= velocityScale;
//			if(useEmitterVelocity > 0.0){\n
//				newVel += ivel.xyz*velocityScale;\n
//			}\n
					return vec4(newVel.xyz, 1.0);\n }\n \n

					// Respawn particle p[gid] at a random emitter position
					void respawn(uint gid){\n lifespan[gid] = newLifespan();\n pos[gid] = newPosition();\n
					//pos[gid] = ipos;\n
					vel[gid] = newVelocity();\n acc[gid] = vec4(0.0, 0.0, 0.0, 1.0);\n }\n);

	return out;
}

//----------------------------------------------------

std::string SNAudioCompPart::initEmitShdr()
{
	std::string out = "#define M_PI 3.1415926535897932384626433832795\n";
	out += "layout(local_size_x = " + std::to_string(work_group_size)
			+ ", local_size_y = 1, local_size_z = 1) in;\n";

	out +=
			STRINGIFY(
					layout(std140, binding=1) uniform EmitParams {\n vec4 ipos;\n vec4 ivel;\n vec4 iacc;\n vec4 prevPos;\n vec4 orientation;\n float radius;\n float velocityScale;\n float averageLifespan;\n float lifespanVariation;\n float emissionAngle;\n float averageVelocity;\n float velocityVariation;\n float useEmitterVelocity;\n
//				int id;\n
//				int count;\n
					};\n

					layout(std140, binding=1) buffer Pos { vec4 pos[]; };\n layout(std140, binding=2) buffer Vel { vec4 vel[]; };\n layout(std140, binding=3) buffer Acc { vec4 acc[]; };\n layout(std140, binding=4) buffer Lifespans { float lifespan[]; };\n layout(std140, binding=5) buffer EmitterIds { int emitterId[]; };\n

					uint rng_state;\n);

	out += getEmitterUtilityCode();
	out += getEmitterCode();

	out +=
			STRINGIFY(
					\n void main(){\n uint gid = gl_GlobalInvocationID.x;\n rng_state = gid;\n

					// Dead?
					if(lifespan[gid] < 0.0){\n
					//if(rng_state % count == id){\n
					respawn(gid);\n
					//	emitterId[gid] = int(id);\n
					//}\n
					}\n });

	return out;
}

//----------------------------------------------------

void SNAudioCompPart::initTfoShdr(TFO* _tfo)
{
	std::string shdr_Header = "#version 430\n#define WORK_GROUP_SIZE "
			+ std::to_string(work_group_size) + "\n";
//	std::string shdr_Header = "#version 430\n#extension GL_EXT_shader_io_blocks : enable\n#define WORK_GROUP_SIZE 128\n";

	std::string vert =
			STRINGIFY(
					layout(std140, binding=1) uniform ShaderParams { mat4 ModelView; mat4 ModelViewProjection; mat4 ProjectionMatrix; vec4 attractor; uint numParticles; float spriteSize; float damping; float noiseFreq; vec3 noiseStrength;

					float time; float persistence; float NOISE_POSITION_SCALE; float NOISE_SCALE; float NOISE_TIME_SCALE; };

					layout(std140, binding=1) buffer Pos { vec4 pos[]; }; layout(std140, binding=2) buffer Lifespans { float lifespan[]; };\n

					uniform vec4[6] colors; uniform mat4 model; uniform int chanNr;

					out gl_PerVertex { vec4 gl_Position; };

					out vec4 rec_position; out vec3 rec_normal; out vec4 rec_texCoord; out vec4 rec_color;

					void main() {
					// expand points to quads without using GS
					int particleID = gl_VertexID >> 2;// 4 vertices per particle

					if (lifespan[particleID] > 0.0) { vec4 particlePos = pos[particleID];

					//map vertex ID to quad vertex
					vec2 quadPos = vec2( ((gl_VertexID - 1) & 2) >> 1, (gl_VertexID & 2) >> 1);

					vec4 particlePosEye = model * particlePos; vec4 vertexPosEye = particlePosEye + vec4((quadPos*2.0-1.0)*spriteSize, 0, 0);

					rec_position = vertexPosEye; rec_normal = vec3(0.0, 0.0, 1.0); rec_texCoord = vec4(quadPos.x, quadPos.y, 0.0, 0.0); rec_color = colors[chanNr]; gl_Position = vertexPosEye; } else { rec_position = vec4(0.0); rec_normal = vec3(0.0, 0.0, 1.0); rec_texCoord = vec4(0.0); rec_color = colors[chanNr]; gl_Position = vec4(0.0); } });

	vert = shdr_Header + vert;

	//

	shdr_Header = "#version 430\n";

	std::string frag =
			STRINGIFY(

					layout(location=0) out vec4 fragColor; void main() { fragColor = vec4(1.0); });

	frag = "// SNAudioCompPart frag\n" + shdr_Header + frag;

	mTfoProg = shCol->addCheckShaderTextNoLink("ParticleCSShaderTfo",
			vert.c_str(), frag.c_str());

	//- Setup TFO ---

	std::vector<std::string> names;
	// copy the standard record attribute names, number depending on hardware
	for (auto i = 0; i < MAX_SEP_REC_BUFS; i++)
		names.push_back(stdRecAttribNames[i]);
	_tfo->setVaryingsToRecord(&names, mTfoProg->getProgram());

	mTfoProg->link();
}

//----------------------------------------------------

void SNAudioCompPart::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	glm::mat4 model;

	if (!inited)
	{
		if (_tfo)
			initTfoShdr(_tfo);
		else
			initDrawShdr();
		inited = true;
	}

	if (_tfo)
	{
		useTfo = true;
		_tfo->end(); // stop tfo damit der shader gewechselt werden kann
	}
	else
	{
		useTfo = false;
	}

	glDisable (GL_STENCIL_TEST);
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
	glDisable (GL_DEPTH_CLAMP);
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive blend

	glActiveTexture (GL_TEXTURE0);

	if (!_tfo)
	{
		// draw particles
		mRenderProg->begin();
		mRenderProg->setUniform4fv("colors", &chanCols[0][0], 6);
		mRenderProg->setUniform1f("alpha", alpha * osc->alpha);
		mRenderProg->setUniform1i("noiseCube", 0);

	}
	else
	{
		mTfoProg->begin();

		_tfo->begin(GL_TRIANGLES);
		sendStdShaderInit(mTfoProg);
		_tfo->disableDepthTest();
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);
		_tfo->setSceneNodeColors(chanCols);
		useTextureUnitInd(0, tex0->getId(), _shader, _tfo);

		mShaderParams.ModelView = glm::translate(glm::mat4(1.f),
				glm::vec3(0.f, 0.f, -2.f));
		//* glm::scale(glm::mat4(1.f), glm::vec3(0.06f, 0.06f, 0.06f));

		mTfoProg->setUniformMatrix4fv("model", &mShaderParams.ModelView[0][0]);
		mTfoProg->setUniform4fv("colors", &chanCols[0][0], 6);
	}

	// bind the buffer for the UBO, and update it with the latest values from the CPU-side struct
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, mUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShaderParams), &mShaderParams);

	if (_tfo)
		_tfo->pauseAndOffsetBuf(GL_TRIANGLES);

	// reference the compute shader buffer, which we will use for the particle
	// wenn kein vao gebunden ist, funktioniert glDrawElements nicht...
	testVAO->bind();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indices->getBuffer());

	for (int i = 0; i < scd->nrChannels; i++)
	{

		if (_tfo)
			mTfoProg->setUniform1i("chanNr", i);
		else
			mRenderProg->setUniform1i("chanNr", i);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_pos[i]->getBuffer());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2,
				m_lifespans[i]->getBuffer());

		glDrawElements(GL_TRIANGLES, mNumParticles * 6, GL_UNSIGNED_INT, 0);

		if (_tfo)
			_tfo->incCounters(mNumParticles * 6);
	}

	testVAO->unbind();

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	if (!_tfo)
		mRenderProg->end();
}

//----------------------------------------------------

SNAudioCompPart::~SNAudioCompPart()
{
}

//----------------------------------------------------

void SNAudioCompPart::update(double time, double dt)
{
	if (time - lastPartUpdt > 0.012 && inited)
	{
		if (!useTfo)
		{
			mShaderParams.spriteSize = spriteSize;
			mShaderParams.initAmt = initAmt;
		}
		else
		{
			//mShaderParams.noiseFreq = std::sqrt( pa->getPitch(0) ) * 0.5f;
			mShaderParams.spriteSize = osc->videoSpeed * 0.005f;
			mShaderParams.initAmt = 0.f;
		}

		// deactivated any shader actually running
		glUseProgram(0);

		for (int i = 0; i < scd->nrChannels; i++)
		{
			pitchMed[i]->update(std::sqrt(pa->getPitch(i)) * 0.005f);

			mShaderParams.ModelView = view * _modelMat;
			mShaderParams.ModelViewProjection = proj * view * _modelMat;
			mShaderParams.ProjectionMatrix = proj;
			mShaderParams.time = time + float(i) * 0.3f;		// 0.0f - 1.f
			mShaderParams.NOISE_POSITION_SCALE = pitchMed[i]->get() * 2.f; // 0.001f - 0.02f
			mShaderParams.NOISE_SCALE = 0.001f;			 // 0.001f - 0.02f
			mShaderParams.NOISE_TIME_SCALE = pitchMed[i]->get();// 0.001f - 2.0f
			mShaderParams.persistence = 0.8f;			 // 0.0f - 1.f

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1,
					m_pos[i]->getBuffer());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2,
					m_vel[i]->getBuffer());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3,
					m_acc[i]->getBuffer());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4,
					m_lifespans[i]->getBuffer());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5,
					m_emitterIds[i]->getBuffer());

			//- emit new particles ---

			emit(time, dt, i);

			//---- update curl noise -

			// bind the buffer for the UBO, and update it with the latest values from the CPU-side struct
			glBindBufferBase(GL_UNIFORM_BUFFER, 1, mUBO);
			glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShaderParams),
					&mShaderParams);

			// Invoke the compute shader to integrate the particles
			glBindProgramPipeline(m_programCurlNoise);
			glDispatchCompute(mNumParticles / work_group_size, 1, 1);

			// We need to block here on compute completion to ensure that the
			// computation is done before we render
			glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
		}

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

		glBindProgramPipeline(0);

		lastPartUpdt = time;
	}
}

//----------------------------------------------------

void SNAudioCompPart::emit(double time, double _dt, unsigned int chanNr)
{
	glm::vec3 pos = glm::vec3(pa->getPllAtPos(chanNr, 0.1f + time * 0.1f),
			pa->getPllAtPos(chanNr, 0.2f + time * 0.1f), -1.f);

	float dt = _dt;
//	float dt = time - prevTime;

	glm::vec3 newVel = (pos - prevPos[chanNr]) * dt;
	glm::vec3 acc = (newVel - prevVel[chanNr]) * dt;

	mEmitParams.averageLifespan = 160.f;	// 1.0 - 600
	mEmitParams.averageVelocity = pa->getMedAmp(chanNr) * 0.1f; // 0.0 - 10.0, streuung nach emission
	//mEmitParams.count = 1;
	mEmitParams.emissionAngle = 0.f;
//	mEmitParams.emissionAngle = M_PI / 6.f;
	mEmitParams.lifespanVariation = 0.f;	// 0.0 - 100.
	mEmitParams.orientation = glm::vec4(1.f, 0.f, 0.f, 0.f);
	mEmitParams.prevPos = glm::vec4(prevPos[chanNr].x, prevPos[chanNr].y,
			prevPos[chanNr].z, 1.f);
	mEmitParams.radius = 0.01f;				// 1.0 - 30.
	mEmitParams.useEmitterVelocity = 1.f;
	mEmitParams.velocityVariation = 30.f;  // 0 - 100
	mEmitParams.ipos = glm::vec4(pos.x, pos.y, pos.z, 1.f);
	mEmitParams.ivel = glm::vec4(newVel.x, newVel.y, newVel.z, 0.f);
	mEmitParams.iacc = glm::vec4(acc.x, acc.y, acc.z, 0.f);

	glBindBufferBase(GL_UNIFORM_BUFFER, 1, eUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, eUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(EmitParams), &mEmitParams);

	glBindProgramPipeline(m_programEmit);
	glDispatchCompute(mNumParticles / work_group_size, 1, 1);
	glMemoryBarrier (GL_ALL_BARRIER_BITS);
	glBindProgramPipeline(0);

	prevTime[chanNr] = time;
	prevPos[chanNr] = pos;
	prevVel[chanNr] = newVel;
}

//----------------------------------------------------

void SNAudioCompPart::reset(glm::vec3 size)
{
	for (auto j = 0; j < scd->nrChannels; j++)
	{
		glm::vec4* pos = m_pos[j]->map();
		for (size_t i = 0; i < mNumParticles; i++)
			pos[i] = glm::vec4(0.f, 0.f, 0.f, 1.0f);
		m_pos[j]->unmap();

		glm::vec4 *vel = m_vel[j]->map();
		for (size_t i = 0; i < mNumParticles; i++)
			vel[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		m_vel[j]->unmap();

		glm::vec4 *acc = m_acc[j]->map();
		for (size_t i = 0; i < mNumParticles; i++)
			acc[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		m_acc[j]->unmap();

		float *lifeSpan = m_lifespans[j]->map();
		for (size_t i = 0; i < mNumParticles; i++)
			lifeSpan[i] = getRandF(0, 120.f);
		m_lifespans[j]->unmap();

		int *emitId = m_emitterIds[j]->map();
		for (size_t i = 0; i < mNumParticles; i++)
			emitId[i] = i;
		m_emitterIds[j]->unmap();
	}
}

//----------------------------------------------------

void SNAudioCompPart::onKey(int key, int scancode, int action, int mods)
{
}
}
