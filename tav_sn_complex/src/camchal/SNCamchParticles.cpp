//
// SNCamchParticles.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  die "update" methode muss aufgerufen werden!!!

#include "SNCamchParticles.h"

#define STRINGIFY(A) #A


namespace tav
{

SNCamchParticles::SNCamchParticles(sceneData* _scd, std::map<std::string, float>* _sceneArgs)
: SceneNode(_scd, _sceneArgs),
  mEnableAttractor(false),
  mReset(false),
  mTime(0.0f),
  alpha(1.f)
{
	shCol = (ShaderCollector*)_scd->shaderCollector;

	// setup chanCols
	getChanCols(&chanCols);

	initShdr();

	// get screen Proportions from roomDim
	propo = _scd->roomDim->x / _scd->roomDim->y;

	addPar("alpha", &alpha);
	addPar("noiseFreq", &noiseFreq);
	addPar("noiseStren", &noiseStren);
	addPar("spriteSize", &spriteSize);
	addPar("initAmt", &initAmt);


	// setup custom matrix
	globalScale = 16.f;
	view = glm::lookAt(
			glm::vec3(0.f, 0.f, globalScale),	// camPos
			glm::vec3(0.f), 			// lookAt
			glm::vec3(0.f, 1.f, 0.f));	// upVec

	//float aspect = float(scd->screenWidth) / float(scd->screenHeight);
	float near = globalScale * 0.1f;
	float far = globalScale * 10.f;
	proj = glm::perspective(0.2f, propo, near, far);


	// vao mit irgendwas drin, damit die sache mit dem shader storage buffer richtig funktioniert
	testVAO = new VAO("position:3f", GL_STATIC_DRAW);
	GLfloat pos[] = { -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.0f };
	testVAO->upload(POSITION, pos, 4);
	GLuint ind[] = { 0, 1, 3, 1, 3, 2 };
	testVAO->setElemIndices(6, ind);

	//create ubo and initialize it with the structure data
	glGenBuffers( 1, &mUBO);
	glBindBuffer( GL_UNIFORM_BUFFER, mUBO);
	glBufferData( GL_UNIFORM_BUFFER, sizeof(ShaderParams), &mShaderParams, GL_STREAM_DRAW);

	// For now, scale back the particle count on mobile.
	mParticles = new tav::GLSLParticleSystemCS(mNumParticles, shCol,  GLSLParticleSystemCS::DRAW_GEN_QUADS);
	//mParticles->randomInit(glm::vec3(propo*2.f, 1.f, 1.f));
	mParticles->randomInitBuf("pos", glm::vec4(propo*2.f, 1.f, 1.f, 0.f), glm::vec4(0.f, 0.f, 0.f, 1.f));
	mParticles->setAging(false);

	std::string pUpdtShdrSrc = initPartShdr();
	mParticles->setUpdateShader(pUpdtShdrSrc.c_str());
	mParticles->setEmitShader();

}

//----------------------------------------------------

std::string SNCamchParticles::initPartShdr()
{
	// version statement kommt von aussen
	std::string shdr_Header = "#version 430\n#define WORK_GROUP_SIZE 128\n";

	std::string src = STRINGIFY(
	layout(std140, binding=1) uniform ShaderParams\n {
		mat4 ModelView;\n
		mat4 ModelViewProjection;\n
		mat4 ProjectionMatrix;\n
		vec4 attractor;\n
		uint numParticles;\n
		float spriteSize;\n
		float damping;\n
		float initAmt;\n
		float noiseFreq;\n
		float noiseStrength;\n
	};\n

	uniform float invNoiseSize;\n
	uniform sampler3D noiseTex3D;\n

	layout( std140, binding=1 ) buffer Pos {\n vec4 pos[];\n };\n

	layout( std140, binding=2 ) buffer Init_Pos {\n vec4 init_pos[];\n };\n

	layout( std140, binding=3 ) buffer Vel {\n vec4 vel[];\n };\n

	layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;\n

	// noise functions
	// returns random value in [-1, 1]
	vec3 noise3f(vec3 p) {
		return texture(noiseTex3D, p * invNoiseSize).xyz;
	}

	// fractal sum
	vec3 fBm3f(vec3 p, int octaves, float lacunarity, float gain) {\n
		float freq = 1.0;
		float amp = 0.5;
		vec3 sum = vec3(0.0);

		for(int i=0; i<octaves; i++) {
			sum += noise3f(p*freq)*amp;
			freq *= lacunarity;
			amp *= gain;
		}

		return sum;
	}

	vec3 attract(vec3 p, vec3 p2)
	{
		const float softeningSquared = 0.01; vec3 v = p2 - p;
		float r2 = dot(v, v);
		r2 += softeningSquared;
		float invDist = 1.0f / sqrt(r2);
		float invDistCubed = invDist *invDist *invDist;
		return v * invDistCubed;
	}

	// compute shader to update particles
	void main() {

		uint i = gl_GlobalInvocationID.x;

		// thread block size may not be exact multiple of number of particles
		if (i >= numParticles) return;

		// read particle position and velocity from buffers
		vec3 p = mix( pos[i].xyz, init_pos[i].xyz, initAmt);\n vec3 v = vel[i].xyz;\n

		v += fBm3f(p*noiseFreq, 4, 2.0, 0.5) * noiseStrength; v += attract(p, attractor.xyz) * attractor.w;

		// integrate
		p += v;\n v *= damping;\n

		// write new values
		pos[i] = vec4(p, 1.0);\n vel[i] = vec4(v, 0.0);\n
	});

	return shdr_Header + src;
}

//----------------------------------------------------

void SNCamchParticles::initShdr()
{
	std::string shdr_Header = "#version 430\n#define WORK_GROUP_SIZE 128\n";
//	std::string shdr_Header = "#version 430\n#extension GL_EXT_shader_io_blocks : enable\n#define WORK_GROUP_SIZE 128\n";

	std::string vert = STRINGIFY(
	layout(std140, binding=1) uniform ShaderParams
	{
		mat4 ModelView;
		mat4 ModelViewProjection;
		mat4 ProjectionMatrix;
		vec4 attractor;
		uint  numParticles;
		float spriteSize;
		float damping;
		float noiseFreq;
		vec3 noiseStrength;
	};

	layout( std140, binding=1 ) buffer Pos { vec4 pos[]; };

	uniform vec4[6] colors;
	uniform sampler3D noiseCube;

	out gl_PerVertex {
		vec4 gl_Position;
	};

	out block {
		vec4 color;
		vec2 texCoord;
	} Out;

	void main() {
		// expand points to quads without using GS
		int particleID = gl_VertexID >> 2; // 4 vertices per particle
		vec4 particlePos = pos[particleID];

		//Out.color = mix(colors[1], colors[2], 1.0 );
		Out.color = mix(colors[1], colors[2], min ( max( texture(noiseCube, particlePos.xyz * 0.1) + 0.3, 0.0), 1.0));
		Out.color.a *= 0.4;

		//map vertex ID to quad vertex
		vec2 quadPos = vec2( ((gl_VertexID - 1) & 2) >> 1, (gl_VertexID & 2) >> 1);

		vec4 particlePosEye = ModelView * particlePos;
		vec4 vertexPosEye = particlePosEye + vec4((quadPos*2.0-1.0)*spriteSize, 0, 0);

		Out.texCoord = quadPos;

		gl_Position = ProjectionMatrix * vertexPosEye;
	});

	vert = shdr_Header +vert;

	//

	shdr_Header = "#version 430\n";
//	shdr_Header = "#version 430\n#extension GL_EXT_shader_io_blocks : enable\n";

	std::string frag = STRINGIFY(
	in block {
		vec4 color;
		vec2 texCoord;
	} In;

	layout(location=0) out vec4 fragColor;
	uniform float alpha;

	void main() {
		// Quick fall-off computation
		float r = length(In.texCoord*2.0-1.0)*3.0;
		float i = exp(-r*r);
		if (i < 0.01) discard;

		fragColor = vec4(In.color.rgb, i * In.color.a * alpha);
	});

	frag = "// SNCamchParticles frag\n"+shdr_Header+frag;

	mRenderProg = shCol->addCheckShaderText("ParticleCSShader3234", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNCamchParticles::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_CLAMP);

	glm::mat4 model = _modelMat; // model pos

	mShaderParams.ModelView = view * model;
	mShaderParams.ModelViewProjection = proj * view * model;
	mShaderParams.ProjectionMatrix = proj;

	// update struct representing UBO
	mShaderParams.numParticles = mParticles->getSize();
//	mShaderParams.ModelView = cp->view_matrix_mat4 * model;
//	mShaderParams.ModelViewProjection = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * model;
//	mShaderParams.ProjectionMatrix = cp->projection_matrix_mat4;
	mShaderParams.noiseFreq = noiseFreq;
	mShaderParams.noiseStrength = noiseStren;
	mShaderParams.spriteSize = spriteSize;
	mShaderParams.initAmt = initAmt;

	if (mEnableAttractor) {
		// move attractor
		const float speed = 0.2f;
		mShaderParams.attractor.x = sin(mTime*speed);
		mShaderParams.attractor.y = sin(mTime*speed*1.3f);
		mShaderParams.attractor.z = cos(mTime*speed);
		mTime += dt;

		mShaderParams.attractor.w = 0.0002f;
	} else {
		mShaderParams.attractor.w = 0.0f;
	}


	glActiveTexture(GL_TEXTURE0);

	// bind the buffer for the UBO, and update it with the latest values from the CPU-side struct
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, mUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShaderParams), &mShaderParams);

	// this buffer is also used by the compute shader inside GLSLParticleSystem
	// make framerate independent
	if (time - lastPartUpdt > 0.016)
	{
		mParticles->update(time, dt);
		lastPartUpdt = time;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive blend
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);          // if using glScissor

	// draw particles
	mRenderProg->begin();
	mRenderProg->setUniform4fv("colors", &chanCols[0][0], 6);
	mRenderProg->setUniform1f("alpha", alpha);
	mRenderProg->setUniform1i("noiseCube", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, mParticles->getNoiseTex());

	// reference the compute shader buffer, which we will use for the particle
	// wenn kein vao gebunden ist, funktioniert glDrawElements nicht...
	testVAO->bind();

//	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  mParticles->getPosBuffer()->getBuffer() );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  mParticles->getBuffer("pos")->getBuffer() );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mParticles->getIndexBuffer()->getBuffer() );

	glDrawElements(GL_TRIANGLES, mParticles->getSize()*6, GL_UNSIGNED_INT, 0);

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	glDisable(GL_BLEND);

	mRenderProg->end();
}

//----------------------------------------------------

SNCamchParticles::~SNCamchParticles()
{}

//----------------------------------------------------

void SNCamchParticles::update(double time, double dt)
{
	if (time - lastTime > 2.0)
	{
		printf("reset\n");
	}

	lastTime = time;
}

//----------------------------------------------------

void SNCamchParticles::onKey(int key, int scancode, int action, int mods)
{}
}
