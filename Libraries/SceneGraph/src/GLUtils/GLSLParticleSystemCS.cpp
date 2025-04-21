#include "GLUtils/GLSLParticleSystemCS.h"

using namespace glm;
using namespace std;

namespace ara {

GLSLParticleSystemCS::GLSLParticleSystemCS(size_t size, const char *shaderPrefix)
    : m_size(size), m_noiseTex(0), m_noiseSize(16), m_updateProg(0), m_shaderPrefix(shaderPrefix),
      work_group_size(128) {
    // Split the velocities and positions, as for now the
    // rendering only cares about positions, so we just index
    // that m_buffer for rendering
    m_pos      = new ShaderBuffer<glm::vec4>(size);
    m_init_pos = new ShaderBuffer<glm::vec4>(size);
    m_vel      = new ShaderBuffer<glm::vec4>(size);
    m_indices  = new ShaderBuffer<uint32_t>(size * 6);

    // the index m_buffer is a classic "two-tri quad" array.
    // This may seem odd, given that the compute m_buffer contains a single
    // vector for each particle.  However, the shader takes care of this
    // by indexing the compute shader m_buffer with a /4.  The value mod 4
    // is used to compute the offset from the vertex site, and each of the
    // four indices in a given quad references the same center point
    uint32_t *indices = m_indices->map();
    for (size_t i = 0; i < m_size; i++) {
        size_t index = i << 2;
        *(indices++) = (uint32_t)index;
        *(indices++) = (uint32_t)index + 1;
        *(indices++) = (uint32_t)index + 2;
        *(indices++) = (uint32_t)index;
        *(indices++) = (uint32_t)index + 2;
        *(indices++) = (uint32_t)index + 3;
    }
    m_indices->unmap();

    nt         = new NoiseTexNV(m_noiseSize, m_noiseSize, m_noiseSize, GL_RGBA8_SNORM);
    m_noiseTex = nt->getTex();

    loadShaders();

    //	reset(0.5f);
}

static inline const char *GetShaderstageName(GLenum target) {
    switch (target) {
        case GL_VERTEX_SHADER: return "VERTEX_SHADER"; break;
        case GL_FRAGMENT_SHADER: return "FRAGMENT_SHADER"; break;
        case GL_COMPUTE_SHADER: return "COMPUTE_SHADER"; break;
    }
    return "";
}

GLuint GLSLParticleSystemCS::createShaderPipelineProgram(GLuint target, const char *src) {
    GLuint object;
    GLint  status;

    const GLchar *fullSrc[2] = {m_shaderPrefix, src};
    object                   = glCreateShaderProgramv(target, 2,
                                                      fullSrc);  // with this command GL_PROGRAM_SEPARABLE is set to true

    {
        GLint logLength;
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &logLength);
        char *log = new char[logLength];
        glGetProgramInfoLog(object, logLength, 0, log);
        //  printf("Shader pipeline program not valid:\n%s\n", log);
        delete[] log;
    }

    glBindProgramPipeline(m_programPipeline);
    glUseProgramStages(m_programPipeline, GL_COMPUTE_SHADER_BIT, object);
    glValidateProgramPipeline(m_programPipeline);
    glGetProgramPipelineiv(m_programPipeline, GL_VALIDATE_STATUS, &status);

    if (status != GL_TRUE) {
        GLint logLength;
        glGetProgramPipelineiv(m_programPipeline, GL_INFO_LOG_LENGTH, &logLength);
        char *log = new char[logLength];
        glGetProgramPipelineInfoLog(m_programPipeline, logLength, 0, log);
        printf("Shader pipeline not valid:\n%s\n", log);
        delete[] log;
    }

    glBindProgramPipeline(0);

    return object;
}

void GLSLParticleSystemCS::loadShaders() {
    if (m_updateProg) {
        glDeleteProgram(m_updateProg);
        m_updateProg = 0;
    }

    glGenProgramPipelines(1, &m_programPipeline);

    // std::string src =
    // loadShadersourceWithUniformTag("ara::Shaders/uniforms.h",
    // "ara::Shaders/particlesCS.glsl");
    std::string src = initShdr();

    m_updateProg = createShaderPipelineProgram(GL_COMPUTE_SHADER, src.c_str());

    glBindProgramPipeline(m_programPipeline);

    GLint loc = glGetUniformLocation(m_updateProg, "invNoiseSize");
    glProgramUniform1f(m_updateProg, loc, 1.0f / m_noiseSize);

    loc = glGetUniformLocation(m_updateProg, "noiseTex3D");
    glProgramUniform1i(m_updateProg, loc, 0);

    glBindProgramPipeline(0);
}

void GLSLParticleSystemCS::reset(glm::vec3 size) {
    glm::vec4 *pos = m_pos->map();
    for (size_t i = 0; i < m_size; i++) {
        pos[i] = glm::vec4(getRandF(-1.f, 1.f) * size.x, getRandF(-1.f, 1.f) * size.y, getRandF(-1.f, 1.f) * size.z, 1.0f);
    }
    m_pos->unmap();

    pos = m_init_pos->map();
    for (size_t i = 0; i < m_size; i++){
        pos[i] = glm::vec4(getRandF(-1.f, 1.f) * size.x, getRandF(-1.f, 1.f) * size.y, getRandF(-1.f, 1.f) * size.z, 1.0f);
    }
    m_init_pos->unmap();

    glm::vec4 *vel = m_vel->map();
    for (size_t i = 0; i < m_size; i++) vel[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    m_vel->unmap();
}

void GLSLParticleSystemCS::update() {
    // deactivated any shader actually running
    glUseProgram(0);

    // Invoke the compute shader to integrate the particles
    glBindProgramPipeline(m_programPipeline);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, m_noiseTex);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_pos->getBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_init_pos->getBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_vel->getBuffer());

    glDispatchCompute((GLuint)m_size / work_group_size, 1, 1);

    // We need to block here on compute completion to ensure that the
    // computation is done before we render
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

    glBindProgramPipeline(0);
}

std::string GLSLParticleSystemCS::initShdr() {
    // version statement comes from outside this method
    std::string shdr_Header = "#define WORK_GROUP_SIZE 128\n";

    std::string src = STRINGIFY(
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
				freq *= lacunarity; amp *= gain;
			} 
			
			return sum; 
		}

		vec3 attract(vec3 p, vec3 p2) { 
			const float softeningSquared = 0.01;
			vec3 v = p2 - p;
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
			vec3 p = mix( pos[i].xyz, init_pos[i].xyz, initAmt);\n
			vec3 v = vel[i].xyz;\n

			v += fBm3f(p*noiseFreq, 4, 2.0, 0.5) * noiseStrength;
			v += attract(p, attractor.xyz) * attractor.w;

			// integrate
			p += v;\n 
			v *= damping;\n

			// write new values
			pos[i] = vec4(p, 1.0);\n 
			vel[i] = vec4(v, 0.0);\n 
		});

    src = shdr_Header + getShaderParUBlock() + src;

    return src;
}

std::string GLSLParticleSystemCS::getShaderParUBlock() {
    return STRINGIFY(layout(std140, binding=1) uniform ShaderParams\n
		{ 
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
		};\n);
}

GLSLParticleSystemCS::~GLSLParticleSystemCS() {
    if (m_updateProg) glDeleteProgram(m_updateProg);
    glDeleteProgramPipelines(1, &m_programPipeline);

    delete nt;
    delete m_pos;
    delete m_init_pos;
    delete m_vel;
    delete m_indices;
}

}  // namespace ara
