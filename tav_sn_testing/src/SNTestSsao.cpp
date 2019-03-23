//
// SNTestSsao.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#define STRINGIFY(A) #A

#include "SNTestSsao.h"

namespace tav
{
SNTestSsao::SNTestSsao(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs), spSize(1.f), perlForce(1.f), randAmt(0.2f),
		posRandSpd(1.f), posRandAmtX(1.f), posRandAmtY(1.f), lastBoundFbo(0)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	ssao = new SSAO(_scd);

	// init osc parameter
	addPar("spSize", &spSize);
	addPar("perlForce", &perlForce);
	addPar("randAmt", &randAmt);
	addPar("posRandSpd", &posRandSpd);
	addPar("posRandAmtX", &posRandAmtX);
	addPar("posRandAmtY", &posRandAmtY);

    propo = _scd->roomDim->x / _scd->roomDim->y;

    // init default VAO
	glGenVertexArrays(1, &defaultVAO);
	glBindVertexArray(defaultVAO);

	texShdr = shCol->getStdTex();
	initShaders();
	initScene();

	glBindVertexArray(0);

	noiseTex = new Noise3DTexGen(shCol,
			true, 4,
			256, 256, 64,
			4.f, 4.f, 16.f);

    quad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
                    glm::vec3(0.f, 0.f, 1.f),
                    0.f, 0.f, 0.f, 1.f);

    // test texture
    litsphereTex = new TextureManager();
    litsphereTex->loadTexture2D(*scd->dataPath+"/textures/litspheres/Unknown-28.jpeg");
}

//----------------------------------------------------

void SNTestSsao::initShaders()
{
	shdr_Header = "#version 430\n";
	initSceneShdr();

	// init compute shader for offseting the positions of the spheres
	std::string src = initComputeUpdtShdr();
	m_updateProg = createShaderPipelineProgram(GL_COMPUTE_SHADER, src.c_str());
}

//----------------------------------------------------

void SNTestSsao::initSceneShdr()
{
	std::string vert = "layout( std140, binding=1 ) buffer modu_pos { vec4 offs_pos[]; };\n";
	vert += "layout( std140, binding=2 ) buffer Vel { vec4 vel[]; };\n";

	vert += "in layout(location= "; vert += std::to_string(VERTEX_POS); vert += ") vec3 pos;\n";
	vert += "in layout(location= "; vert += std::to_string(VERTEX_NORMAL); vert += ") vec3 normal;\n";
	vert += "in layout(location= "; vert += std::to_string(VERTEX_COLOR); vert += ") vec4 color;\n";

	vert += STRINGIFY(
	out Interpolants {\n
		vec3 pos;\n
		vec3 normal;\n
		flat vec4 color;\n
	} OUT;\n

	uniform uint sphereIndOffs;
	uniform float sphereSize;
	uniform mat4 viewProjMatrix;

	void main() {\n

		uint sphereNr = gl_VertexID / sphereIndOffs;
		vec3 modPos = (pos * sphereSize * vel[sphereNr].x) + offs_pos[sphereNr].xyz;
		gl_Position = viewProjMatrix * vec4(modPos,1);\n
		OUT.pos = modPos;\n
		OUT.normal = normal;\n
		OUT.color = color;\n
	});

	vert = "// SNTestSsao Scene Shader vertex shader\n" + shdr_Header + vert;

	

	std::string frag = STRINGIFY(in Interpolants {
		vec3 pos;
		vec3 normal;
		flat vec4 color;
	} IN;

	layout(location=0,index=0) out vec4 out_Color;

	void main()
	{
		vec3  light = normalize(vec3(-1,2,1));
		float intensity = dot(normalize(IN.normal), light) * 0.5 + 0.5;
		vec4  color = IN.color * mix(vec4(0, 0, 0, 0), vec4(1,1,1,0), intensity);

		out_Color = color;
	});

	frag = "// SNTestSsao Scene Shader vertex shader\n" + shdr_Header + frag;

	draw_scene = shCol->addCheckShaderText("SNTestSsaoDrawScene", vert.c_str(), frag.c_str());
}


std::string SNTestSsao::initComputeUpdtShdr()
{
	// version statement kommt von aussen
	std::string shdr_Header = "#define WORK_GROUP_SIZE 128\n";

	std::string src = STRINGIFY(
	uniform sampler3D noiseTex3D;\n
	uniform uint totNumParticles;\n
	uniform vec3 numPart;\n
	uniform float time;\n
	uniform float perlForce;\n
	uniform float randAmt;\n

	layout( std140, binding=1 ) buffer Modu_pos { vec4 offs_pos[];\n };\n
	layout( std140, binding=2 ) buffer Ref_pos { vec4 r_pos[];\n };\n
	layout( std140, binding=3 ) buffer Vel { vec4 vel[];\n };\n

	layout(local_size_x = 128,  local_size_y = 1, local_size_z = 1) in;\n


	// compute shader to update particles
	void main() {

		uint i = gl_GlobalInvocationID.x;

		// thread block size may not be exact multiple of number of particles
		if (i >= totNumParticles) return;

		vec3 normPos = vec3(mod(float(i), numPart.x) / numPart.x,
							mod(float(i) / numPart.x, numPart.y) / numPart.y,
							mod(float(i) / (numPart.x * numPart.y), numPart.z) / numPart.z);

		// read particle position and velocity from buffers
		vec3 offsCo = vec3(normPos.x + time * 0.2,
						   normPos.y + time * 0.23,
						   normPos.z * 0.8 + time * 0.12);

		vec4 perlOffs = texture(noiseTex3D, offsCo);
		perlOffs -= vec4(0.25, 0.25, 0.25, 0); // -0.25 bis 0.25
		perlOffs.z *=  10.0;
		perlOffs.z -=  2.0;

		// read particle position and velocity from buffers
		vec3 p = r_pos[i].xyz + perlOffs.xyz * perlForce;\n

		// write new values
		offs_pos[i] = vec4(p, 1.0);\n
		vel[i].x = 2.0;\n

		// write new values
		offs_pos[i] = vec4(mix(r_pos[i].xyz, offs_pos[i].xyz, randAmt), 0.0);\n
	});

	src = shdr_Header + src;

	return src;
}


GLuint SNTestSsao::createShaderPipelineProgram(GLuint target, const char* src)
{
	GLuint object;
	GLint status;
	std::string header = "#version 430\n";

    glUseProgram(0); // MAKE sure no shader is active;
	glGenProgramPipelines(1, &m_programPipeline);

	const GLchar* fullSrc[2] = { header.c_str(), src };
	object = glCreateShaderProgramv( target, 2, fullSrc); // with this command GL_PROGRAM_SEPARABLE is set to true												// and a program object is generated and returned

	{
		GLint logLength;
		glGetProgramiv(object, GL_INFO_LOG_LENGTH, &logLength);
	    char *log = new char [logLength];
	    glGetProgramInfoLog(object, logLength, 0, log);
	    printf("%s\n", log);
	    delete [] log;
	}

	glBindProgramPipeline(m_programPipeline);
	glUseProgramStages(m_programPipeline, GL_COMPUTE_SHADER_BIT, object);
	glValidateProgramPipeline(m_programPipeline);
	glGetProgramPipelineiv(m_programPipeline, GL_VALIDATE_STATUS, &status);

	if (status != GL_TRUE) {
		GLint logLength;

		glGetProgramPipelineiv(m_programPipeline, GL_INFO_LOG_LENGTH, &logLength);
		getGlError();

		char *log = new char [logLength];

		glGetProgramPipelineInfoLog(m_programPipeline, logLength, 0, log);
		getGlError();

		printf("SNTestSsao::createShaderPipelineProgram Error: Shader pipeline not valid:\n%s\n", log);
		delete [] log;
	}

	glBindProgramPipeline(0);

	return object;
}

//----------------------------------------------------

void SNTestSsao::updateOffsPos(double time)
{
	// deactivated any shader actually running
	glUseProgram(0);

	// Invoke the compute shader to integrate the particles
	//glUseProgram(m_updateProg);
	glBindProgramPipeline(m_programPipeline);


	GLint loc = glGetUniformLocation(m_updateProg, "time");
	glProgramUniform1f(m_updateProg, loc, float(time * 0.06));

	loc = glGetUniformLocation(m_updateProg, "totNumParticles");
	glProgramUniform1ui(m_updateProg, loc, gridX * gridY * gridZ);

	loc = glGetUniformLocation(m_updateProg, "numPart");
	glProgramUniform3f(m_updateProg, loc, float(gridX), float(gridY), float(gridZ));

	loc = glGetUniformLocation(m_updateProg, "noiseTex3D");
	glProgramUniform1i(m_updateProg, loc, 0);

	loc = glGetUniformLocation(m_updateProg, "perlForce");
	glProgramUniform1f(m_updateProg, loc,  perlForce);

	loc = glGetUniformLocation(m_updateProg, "randAmt");
	glProgramUniform1f(m_updateProg, loc, randAmt);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, noiseTex->getTex());

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  modu_pos->getBuffer() );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2,  ref_pos->getBuffer() );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 3,  m_vel->getBuffer() );

	// workgroup size manually set to 128, dirty... must be the same as inside shader
	glDispatchCompute( (gridX*gridY*gridZ) / 128 +1, 1,  1 );

	// We need to block here on compute completion to ensure that the
	// computation is done before we render
	glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  0 );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2,  0 );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 3,  0 );
	glBindProgramPipeline(0);
}


// bau die geometrie zusammen
bool SNTestSsao::initScene()
{
	// Shader Storage for Position manipulation
	modu_pos = new ShaderBuffer<glm::vec4>(gridX * gridY * gridZ);
	glm::vec4 *pos = modu_pos->map();
	for(size_t i=0; i<gridX * gridY * gridZ; i++)
		pos[i] = glm::vec4(0.f, 0.f, 0.f, 0.f);
	modu_pos->unmap();

	m_vel = new ShaderBuffer<glm::vec4>(gridX * gridY * gridZ);
	glm::vec4 *vel = m_vel->map();
	for(size_t i=0; i<gridX * gridY * gridZ; i++) {
		vel[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}
	m_vel->unmap();

	// Shader Storage for Position manipulation
	ref_pos = new ShaderBuffer<glm::vec4>(gridX * gridY * gridZ);
	glm::vec4 *ref_pos_ptr = ref_pos->map();

	// Scene Geometry
	tav::geometry::Mesh<tav::Vertex>  scene;

	int sphereDimWH = 40;
	int sphereDimZ = 40;

	sceneObjects = 0;

	for (int z=0; z<gridZ; z++)
	{
		for (int y=0; y<gridY; y++)
		{
			for (int x=0; x<gridX; x++)
			{
				glm::vec4 color = glm::vec4(1.f);

				// groesse entspricht einem grid
				glm::vec3 size = glm::vec3(1.f / float(gridY),
						1.f / float(gridY),
						1.f / float(gridY));

				// position in grid einheiten
				glm::vec3 pos(float(x), float(y), float(-z) * 2.f);

				// verschiebe ein halbes grids nach links unten
				pos -=  glm::vec3( gridX/2, gridY/2, gridY/2  );

				// skaliere die position auf die definierte gesamt groesse des grids
				pos.x /=  float(gridX) * 0.5f;
				pos.x *= propo;
				pos.y /=  float(gridY) * 0.5f;
				pos.z /=  float(gridY);

				ref_pos_ptr[z*gridY*gridX + y*gridX +x] = glm::vec4(pos.x, pos.y, pos.z, 0.f);

				glm::mat4  matrix    = glm::mat4(1.f);
				matrix = glm::scale(matrix, size);

				uint  oldverts  = scene.getVerticesCount();
				uint  oldinds   = scene.getTriangleIndicesCount();

		//		tav::geometry::Sphere<tav::Vertex>::add(scene, matrix, sphereDimWH, sphereDimZ);
				tav::geometry::Box<tav::Vertex>::add(scene,matrix,2,2,2);

				if(x==0 && y==0) sphereIndOffs = scene.getVerticesCount() - oldverts;

				for (uint v = oldverts; v < scene.getVerticesCount(); v++){
					scene.m_vertices[v].color = color;
				}
			}
		}

		sceneObjects++;
	}

	ref_pos->unmap();

	sceneTriangleIndices = scene.getTriangleIndicesCount();


	// default vao sollte noch gebunden sein sonst gl error

	if (scene_ibo) glDeleteBuffers(1, &scene_ibo);
	glGenBuffers(1,&scene_ibo);
	glNamedBufferStorageEXT(scene_ibo, scene.getTriangleIndicesSize(), &scene.m_indicesTriangles[0], 0);

	if (scene_ibo) glDeleteBuffers(1, &scene_vbo);
	glGenBuffers(1, &scene_vbo);

	glBindBuffer(GL_ARRAY_BUFFER, scene_vbo);
	glNamedBufferStorageEXT(scene_vbo, scene.getVerticesSize(), &scene.m_vertices[0], 0);

	glVertexAttribFormat(VERTEX_COLOR,  4, GL_FLOAT, GL_FALSE,  offsetof(tav::Vertex, color));
	glVertexAttribBinding(VERTEX_COLOR, 0);
	glVertexAttribFormat(VERTEX_POS,    3, GL_FLOAT, GL_FALSE,  offsetof(tav::Vertex, position));
	glVertexAttribBinding(VERTEX_POS,   0);
	glVertexAttribFormat(VERTEX_NORMAL, 3, GL_FLOAT, GL_FALSE,  offsetof(tav::Vertex, normal));
	glVertexAttribBinding(VERTEX_NORMAL,0);

	return true;
}


//----------------------------------------------------

void SNTestSsao::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	// update offset positions via copute shader
	updateOffsPos(time);

//	ssao->bind();
//	ssao->clear();

	glm::mat4 m_pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat;

	draw_scene->begin();
	draw_scene->setUniform1ui("sphereIndOffs", sphereIndOffs);
	draw_scene->setUniform1f("sphereSize", 0.25f);
	draw_scene->setUniformMatrix4fv("viewProjMatrix", &m_pvm[0][0]);

	glBindVertexArray(defaultVAO);               // create VAO, assign the name and bind that array

	// bind position offset buffer
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, modu_pos->getBuffer());
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2, m_vel->getBuffer());

	glBindVertexBuffer(0, scene_vbo, 0, sizeof(tav::Vertex));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene_ibo);

	glEnableVertexAttribArray(VERTEX_POS);
	glEnableVertexAttribArray(VERTEX_NORMAL);
	glEnableVertexAttribArray(VERTEX_COLOR);

	glDrawElements(GL_TRIANGLES, sceneTriangleIndices, GL_UNSIGNED_INT, NV_BUFFER_OFFSET(0));

	glDisableVertexAttribArray(VERTEX_POS);
	glDisableVertexAttribArray(VERTEX_NORMAL);
	glDisableVertexAttribArray(VERTEX_COLOR);

	glBindVertexBuffer(0,0,0,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, 0);
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2, 0);

//	ssao->proc(cp);
//	ssao->drawBlit(cp);
}


//----------------------------------------------------

void SNTestSsao::update(double time, double dt)
{

}



SNTestSsao::~SNTestSsao()
{ }

}
