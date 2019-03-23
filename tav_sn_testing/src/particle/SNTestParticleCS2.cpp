//
// SNTestParticleCS2.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  die "update" methode muss aufgerufen werden!!!

#include "SNTestParticleCS2.h"

#define STRINGIFY(A) #A


namespace tav
{

SNTestParticleCS2::SNTestParticleCS2(sceneData* _scd, std::map<std::string, float>* _sceneArgs)
: SceneNode(_scd, _sceneArgs),
  mEnableAttractor(false),
  mReset(false),
  mTime(0.0f),
  alpha(1.f)
{
	shCol = (ShaderCollector*)_scd->shaderCollector;

	// setup chanCols
	getChanCols(&chanCols);

	initDrawShdr();

	// get screen Proportions from roomDim
	propo = _scd->roomDim->x / _scd->roomDim->y;

	addPar("alpha", &alpha);
	addPar("spriteSize", &spriteSize);
	addPar("initAmt", &initAmt);

	// vao mit irgendwas drin, damit die sache mit dem shader storage buffer richtig funktioniert
	testVAO = new VAO("position:3f", GL_STATIC_DRAW);
	GLfloat pos[] = { -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.0f };
	testVAO->upload(POSITION, pos, 4);
	GLuint ind[] = { 0, 1, 3, 1, 3, 2 };
	testVAO->setElemIndices(6, ind);

	//create ubo and initialize it with the structure data
	glGenBuffers(1, &mUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderParams), &mShaderParams, GL_STREAM_DRAW);

	//create simple single-vertex VBO
	float vtx_data[] = { 0.0f, 0.0f, 0.0f, 1.0f};
	glGenBuffers(1, &mVBO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_data), vtx_data, GL_STATIC_DRAW);

	// load Test Emit Texture
	testEmitTex = new TextureManager();
	testEmitTex->loadTexture2D(*(_scd->dataPath)+"/textures/test_emit_form.png", 1);

	// Init ParticleSystem
	// For now, scale back the particle count on mobile.
	mParticles = new tav::GLSLParticleSystemCS(1<<16, shCol, GLSLParticleSystemCS::DRAW_GEN_QUADS);
	mParticles->addBuffer("vel");
	mParticles->zero();	// all buffers to zero
	mParticles->setUpdateShader();
	mParticles->setEmitShader();

	initPars = new glm::vec4[2];
}

//----------------------------------------------------

void SNTestParticleCS2::initDrawShdr()
{
	std::string shdr_Header = "#version 430\n";
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

		Out.color = vec4(1.0, 1.0, 1.0, min(particlePos.a, 1.0));

		//map vertex ID to quad vertex
		vec2 quadPos = vec2( ((gl_VertexID - 1) & 2) >> 1, (gl_VertexID & 2) >> 1);

		vec4 particlePosEye = ModelView * vec4(particlePos.xyz, 1.0);
		vec4 vertexPosEye = particlePosEye + vec4((quadPos*2.0-1.0)*spriteSize, 0, 0);

		Out.texCoord = quadPos;

		gl_Position = ProjectionMatrix * vertexPosEye;
	});

	vert = shdr_Header +vert;

	//----------------------------------------------------------------------------

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
		if (i < 0.01 || In.color.a < 0.01 ) discard;

		fragColor = vec4(In.color.rgb, i * In.color.a * alpha);
	});

	frag = "// SNTestParticleCS2 frag\n"+shdr_Header+frag;

	mRenderProg = shCol->addCheckShaderText("ParticleCSShader3234", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNTestParticleCS2::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_CLAMP);

	mShaderParams.ModelView = cp->view_matrix_mat4;
	mShaderParams.ModelViewProjection = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat;
	mShaderParams.ProjectionMatrix = proj;

	// update struct representing UBO
	mShaderParams.numParticles = mParticles->getSize();
	mShaderParams.spriteSize = spriteSize;
	mShaderParams.initAmt = initAmt;

	if (mEnableAttractor)
	{
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
	if (time - lastPartUpdt > 0.016){
		mParticles->update(time, dt);
		lastPartUpdt = time;
	}

	if (time - lastEmit > 0.016)
	{
		// emit particles
		initPars[0] = glm::vec4(0.f, -0.4f, 0.f, 2.f); // pos: last parameter = lifetime
		initPars[1] = glm::vec4(0.f, 0.f, 0.f, 0.f);
		//initPars[1] = glm::vec4(getRandF(-0.0005f, 0.0005f), getRandF(0.0005f, 0.001f), 0.f, 0.f);
		//mParticles->emit(500, initPars);

		mParticles->emit(16, initPars, testEmitTex->getId(), testEmitTex->getWidth(),
				testEmitTex->getHeight());

		lastEmit = time;
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

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  mParticles->getBuffer("pos")->getBuffer() );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mParticles->getIndexBuffer()->getBuffer() );

	glDrawElements(GL_TRIANGLES, mParticles->getSize()*6, GL_UNSIGNED_INT, 0);

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	glDisable(GL_BLEND);

	mRenderProg->end();
}

//----------------------------------------------------

SNTestParticleCS2::~SNTestParticleCS2()
{
	delete testVAO;
	delete mParticles;
	delete initPars;
}

//----------------------------------------------------

void SNTestParticleCS2::update(double time, double dt)
{}

//----------------------------------------------------

void SNTestParticleCS2::onKey(int key, int scancode, int action, int mods)
{}
}
