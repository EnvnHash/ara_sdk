//
// SNParticles.cpp
//
//  Created by Sven Hahne, last modified on 22.08.17
//
//  the "update" method has to be called!!!

#include "SceneNodes/SNParticles.h"

#include <GLBase.h>

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SNParticles::SNParticles(sceneData* sd)
    : SceneNode(sd), mEnableAttractor(true), mReset(false), mTime(0.0f), alpha(1.f), noiseFreq(10.f),
      noiseStren(0.002f), spriteSize(0.025f), initAmt(0.f), oldReset(0), lastPartUpdt(0.0), mNumParticles(1 << 17) {
    // get screen Proportions from roomDim
    propo = s_sd->roomDim->x / s_sd->roomDim->y;

    // addPar("alpha", &alpha);
    addPar("noiseFreq", &noiseFreq);
    addPar("noiseStren", &noiseStren);
    addPar("spriteSize", &spriteSize);
    addPar("initAmt", &initAmt);

    // setup custom matrix
    globalScale = 16.f;
    view        = glm::lookAt(glm::vec3(0.f, 0.f, globalScale),  // s_camPos
                              glm::vec3(0.f),                    // s_lookAt
                              glm::vec3(0.f, 1.f, 0.f));         // upVec

    // float aspect = float(s_sd->winViewport.z) / float(s_sd->winViewport.w);
    float f_near = 0.1f;
    float f_far  = globalScale * 10.f;
    proj         = glm::perspective(0.2f, propo, f_near, f_far);

    // default vao with whatever, needed for the shader storage buffers
    testVAO       = make_unique<VAO>("position:3f", GL_STATIC_DRAW);
    GLfloat pos[] = {-0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.0f};
    testVAO->upload(CoordType::Position, pos, 4);
    GLuint ind[] = {0, 1, 3, 1, 3, 2};
    testVAO->setElemIndices(6, ind);

    // create ubo and initialize it with the structure data
    glGenBuffers(1, &mUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderParams), &mShaderParams, GL_STREAM_DRAW);

    // create simple single-vertex VBO
    float vtx_data[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_data), vtx_data, GL_STATIC_DRAW);

    // For now, scale back the particle count on mobile.
    mParticles = make_unique<GLSLParticleSystemCS>(mNumParticles, "#version 430\n");
    mParticles->reset(glm::vec3(propo, 1.f, 1.f));

    nrPartColors     = 6;
    partColors[0][0] = 0.3f;
    partColors[0][1] = 0.5f;
    partColors[0][2] = 0.8f;
    partColors[0][3] = 1.f;
    partColors[1][0] = 0.3f;
    partColors[1][1] = 0.53f;
    partColors[1][2] = 0.81f;
    partColors[1][3] = 1.f;
    partColors[2][0] = 0.7f;
    partColors[2][1] = 0.23f;
    partColors[2][2] = 0.41f;
    partColors[2][3] = 1.f;
    partColors[3][0] = 0.3f;
    partColors[3][1] = 0.75f;
    partColors[3][2] = 0.8f;
    partColors[3][3] = 1.f;
    partColors[4][0] = 0.2f;
    partColors[4][1] = 0.33f;
    partColors[4][2] = 0.21f;
    partColors[4][3] = 1.f;
    partColors[5][0] = 0.1f;
    partColors[5][1] = 0.56f;
    partColors[5][2] = 0.61f;
    partColors[5][3] = 1.f;
}

void SNParticles::initShdr() {
    std::string shdr_Header = "#version 430\n#define WORK_GROUP_SIZE 128\n";
    //	std::string shdr_Header = "#version 430\n#extension
    // GL_EXT_shader_io_blocks : enable\n#define WORK_GROUP_SIZE 128\n";

    std::string vert = STRINGIFY(
	layout( std140, binding=1 ) buffer Pos { vec4 pos[]; };\n
	\n
	uniform vec4[6] colors;\n
	uniform sampler3D noiseCube;\n
	\n
	out gl_PerVertex {\n
		vec4 gl_Position;\n
	};\n
	\n
	out block {\n
		vec4 color;\n
		vec2 texCoord;\n
	} Out;\n
	\n
	void main() {\n
		// expand points to quads without using GS
		int particleID = gl_VertexID >> 2;\n // 4 vertices per particle
		vec4 particlePos = pos[particleID];\n
		particlePos.x *= 3.0;\n
		particlePos.z *= 3.0;\n
		//particlePos.z -= 0.5;\n
		\n
		//Out.color = mix(colors[1], colors[2], 1.0 );
		Out.color = mix(colors[1], colors[2], min ( max( texture(noiseCube, particlePos.xyz * 0.1) + 0.3, 0.0), 1.0));\n
		Out.color.a *= 0.2;\n
		\n
		//map vertex ID to quad vertex\n
		vec2 quadPos = vec2( float(((gl_VertexID - 1) & 2) >> 1), float((gl_VertexID & 2) >> 1));\n
		Out.texCoord = quadPos;\n
		\n
		quadPos = quadPos * 2.0 - 1.0;
		\n
		vec4 particlePosEye = ModelView * particlePos;\n
		vec4 vertexPosEye = particlePosEye + vec4(quadPos * spriteSize, 0, 0);\n
		\n
		gl_Position = ProjectionMatrix * vertexPosEye;\n
	});

    vert = shdr_Header + mParticles->getShaderParUBlock() + vert;

    //

    //	shdr_Header = "#version 430\nextension GL_EXT_shader_io_blocks :
    // enable\n";
    shdr_Header = "#version 430\n";

    std::string frag = STRINGIFY(
	in block {\n
		vec4 color;\n
		vec2 texCoord;\n
	} In;\n
	\n
	layout(location=0) out vec4 fragColor;\n
	uniform float alpha;\n
	\n
	void main() {\n
		// Quick fall-off computation
		float r = length(In.texCoord*2.0-1.0)*3.0;\n
		float i = exp(-r*r);\n
		i *= i*i;\n
		if (i < 0.01) discard;\n
		\n
		fragColor = vec4(In.color.rgb * 1.4, i * In.color.a * alpha);\n
	});

    frag = shdr_Header + "// SNParticles frag\n" + frag;

    mRenderProg = shCol->add("ParticleCSShader3234", vert, frag);
}

void SNParticles::draw(double time, double dt, CameraSet* cs, Shaders* _shader, renderPass _pass, TFO* _tfo) {
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_CLAMP);
    glActiveTexture(GL_TEXTURE0);

    mShaderParams.ModelView           = cs->getModelMatr() * model;
    mShaderParams.ModelViewProjection = cs->getProjectionMatr() * cs->getViewMatr() * model;
    mShaderParams.ProjectionMatrix    = cs->getProjectionMatr();

    // bind the buffer for the UBO, and update it with the latest values from
    // the CPU-side struct
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, mUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShaderParams), &mShaderParams);

    // this buffer is also used by the compute shader inside GLSLParticleSystem
    // make framerate independent
    if (time - lastPartUpdt > 0.016) {
        mParticles->update();
        lastPartUpdt = time;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive blend
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);  // if using glScissor

    // draw particles
    mRenderProg->begin();
    mRenderProg->setUniform4fv("colors", &partColors[0][0], 6);
    mRenderProg->setUniform1f("alpha", alpha);
    mRenderProg->setUniform1i("noiseCube", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, mParticles->getNoiseTex());

    // reference the compute shader buffer, which we will use for the particle
    // if there is no vao bound, glDrawElements doesn't work...
    testVAO->bind();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mParticles->getPosBuffer()->getBuffer());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mParticles->getIndexBuffer()->getBuffer());

    glDrawElements(GL_TRIANGLES, (GLsizei)mParticles->getSize() * 6, GL_UNSIGNED_INT, 0);

    mRenderProg->end();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDisable(GL_BLEND);
}

void SNParticles::update(double time, double dt) {
    if (!shCol) {
        shCol = &m_glbase->shaderCollector();
        initShdr();
    }

    model = glm::rotate(float(M_PI * time * 0.02), glm::vec3(0.f, 1.f, 0.f)) * glm::translate(glm::vec3(0.f, 0.f, 0.f));

    // update struct representing UBO
    mShaderParams.numParticles  = (uint)mParticles->getSize();
    mShaderParams.noiseStrength = noiseStren + float(sin(time * 0.02f)) * 0.0008f;
    mShaderParams.spriteSize    = spriteSize;
    initAmt                     = float(sin(time * 0.125)) * 0.0025f + 0.0025f;
    mShaderParams.initAmt       = initAmt;
    mShaderParams.noiseFreq     = noiseFreq + float(sin(time * 0.02)) * 0.5f;

    if (mEnableAttractor) {
        // move attractor
        const float speed         = 0.1f;
        mShaderParams.attractor.x = sin(mTime * speed + (float)sin(time * 0.01) * 0.01f);
        mShaderParams.attractor.y = sin(mTime * speed * 1.3f);
        mShaderParams.attractor.z = cos(mTime * speed);
        mTime += (float)dt;

        mShaderParams.attractor.w = 0.00002f;
    } else {
        mShaderParams.attractor.w = 0.0f;
    }
}

void SNParticles::onKey(int key, int scancode, int action, int mods) {}

SNParticles::~SNParticles() {
    glDeleteBuffers(1, &mUBO);
    glDeleteBuffers(1, &mVBO);
}

}  // namespace ara
